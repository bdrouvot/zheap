/*-------------------------------------------------------------------------
 *
 * undoactionxlog.c
 *	  WAL replay logic for undo actions.
 *
 *
 * Portions Copyright (c) 1996-2017, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/access/undo/undoactionxlog.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/undoaction_xlog.h"
#include "access/xlog.h"
#include "access/xlogutils.h"
#include "access/zheap.h"

static void
undo_xlog_insert(XLogReaderState *record)
{
	XLogRecPtr	lsn = record->EndRecPtr;
	xl_undo_insert *xlrec = (xl_undo_insert *) XLogRecGetData(record);
	Buffer		buffer;
	Page		page;
	ItemId		lp;
	XLogRedoAction action;

	action = XLogReadBufferForRedo(record, 0, &buffer);
	if (action == BLK_NEEDS_REDO)
	{
		page = BufferGetPage(buffer);

		lp = PageGetItemId(page, xlrec->offnum);
		if (xlrec->relhasindex)
		{
			ItemIdSetDead(lp);
		}
		else
		{
			ItemIdSetUnused(lp);
			/* Set hint bit for ZPageAddItem */
			PageSetHasFreeLinePointers(page);
		}

		PageSetLSN(BufferGetPage(buffer), lsn);
		MarkBufferDirty(buffer);
	}
	if (BufferIsValid(buffer))
		UnlockReleaseBuffer(buffer);
}

void
undoaction_redo(XLogReaderState *record)
{
	uint8		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;

	switch (info)
	{
		case XLOG_UNDO_INSERT:
			undo_xlog_insert(record);
			break;
		default:
			elog(PANIC, "undoaction_redo: unknown op code %u", info);
	}
}
