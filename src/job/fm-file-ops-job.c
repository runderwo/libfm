/*
 *      fm-file-ops-job.c
 *      
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include "fm-file-ops-job.h"
#include "fm-file-ops-job-xfer.h"
#include "fm-file-ops-job-delete.h"

enum
{
	CUR_FILE,
	PERCENT,
	N_SIGNALS
};

static guint signals[N_SIGNALS];

static void fm_file_ops_job_finalize  			(GObject *object);

static gboolean fm_file_ops_job_run(FmJob* fm_job);

/* funcs for io jobs */
static gboolean chmod_files(FmFileOpsJob* job);
static gboolean chown_files(FmFileOpsJob* job);


G_DEFINE_TYPE(FmFileOpsJob, fm_file_ops_job, FM_TYPE_JOB);

static void fm_file_ops_job_class_init(FmFileOpsJobClass *klass)
{
	GObjectClass *g_object_class;
	FmJobClass* job_class;
	g_object_class = G_OBJECT_CLASS(klass);
	g_object_class->finalize = fm_file_ops_job_finalize;

	job_class = FM_JOB_CLASS(klass);
	job_class->run = fm_file_ops_job_run;
	job_class->finished = NULL;

    signals[CUR_FILE] =
        g_signal_new( "cur-file",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmFileOpsJobClass, cur_file ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__POINTER,
                      G_TYPE_NONE, 1, G_TYPE_POINTER );

    signals[PERCENT] =
        g_signal_new( "percent",
                      G_TYPE_FROM_CLASS ( klass ),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET ( FmFileOpsJobClass, percent ),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__UINT,
                      G_TYPE_NONE, 1, G_TYPE_UINT );
}


static void fm_file_ops_job_finalize(GObject *object)
{
	FmFileOpsJob *self;

	g_return_if_fail(object != NULL);
	g_return_if_fail(IS_FM_FILE_OPS_JOB(object));

	self = FM_FILE_OPS_JOB(object);

	if(self->srcs);
		fm_list_unref(self->srcs);
	if(self->dest)
		fm_path_unref(self->dest);

	G_OBJECT_CLASS(fm_file_ops_job_parent_class)->finalize(object);
}


static void fm_file_ops_job_init(FmFileOpsJob *self)
{
	fm_job_init_cancellable((FmJob*)self);
}


FmJob *fm_file_ops_job_new(FmFileOpType type, FmPathList* files)
{
	FmFileOpsJob* job = (FmFileOpsJob*)g_object_new(FM_FILE_OPS_JOB_TYPE, NULL);
	job->srcs = fm_list_ref(files);
	job->type = type;
	return (FmJob*)job;
}

gboolean fm_file_ops_job_run(FmJob* fm_job)
{
	FmFileOpsJob* job = (FmFileOpsJob*)fm_job;
	FmPath* tmp;

	switch(job->type)
	{
	case FM_FILE_OP_COPY:
		fm_file_ops_job_copy_run(job);
		break;
	case FM_FILE_OP_MOVE:
		fm_file_ops_job_move_run(job);
		break;
	case FM_FILE_OP_TRASH:
		fm_file_ops_job_trash_run(job);
		break;
	case FM_FILE_OP_DELETE:
		fm_file_ops_job_delete_run(job);
		break;
	case FM_FILE_OP_CHMOD:
		chmod_files(job);
		break;
	case FM_FILE_OP_CHOWN:
		chown_files(job);
		break;
	}
	return TRUE;
}


void fm_file_ops_job_set_dest(FmFileOpsJob* job, FmPath* dest)
{
	job->dest = fm_path_ref(dest);
}

static gboolean on_cancelled(FmFileOpsJob* job)
{
	fm_job_emit_cancelled((FmJob*)job);
	return FALSE;
}

gboolean chmod_files(FmFileOpsJob* job)
{

	return FALSE;
}

gboolean chown_files(FmFileOpsJob* job)
{

	return FALSE;
}

static void emit_cur_file(FmFileOpsJob* job, const char* cur_file)
{
	g_signal_emit(job, signals[CUR_FILE], 0, cur_file);
}

void fm_file_ops_job_emit_cur_file(FmFileOpsJob* job, const char* cur_file)
{
	fm_job_call_main_thread(job, emit_cur_file, cur_file);
}

static void emit_percent(FmFileOpsJob* job, gpointer percent)
{
	g_signal_emit(job, signals[PERCENT], 0, (guint)percent);
}

void fm_file_ops_job_emit_percent(FmFileOpsJob* job)
{
    guint percent = (guint)(job->finished + job->current) * 100 / job->total;
    if( percent > job->percent )
    {
    	fm_job_call_main_thread(job, emit_percent, (gpointer)percent);
        job->percent = percent;
    }
}

