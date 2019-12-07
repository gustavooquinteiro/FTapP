#ifndef _SEG_QUEUE_H
#define _SEG_QUEUE_H

typedef struct segment Segment;

typedef struct seg_queue SegmentQueue;

SegmentQueue* new_seg_queue();

void seg_queue_push(SegmentQueue*, Segment*);

Segment* seg_queue_pop(SegmentQueue*);

Segment* seg_queue_front(SegmentQueue* q);

int seg_queue_isempty(SegmentQueue*);

void seg_clear_queue(SegmentQueue * queue);


#endif
