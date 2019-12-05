#ifndef _PKG_QUEUE_H
#define _PKG_QUEUE_H

typedef struct package Package;

typedef struct pkg_queue PackageQueue;

PackageQueue* new_pkg_queue();

void pkg_queue_push(PackageQueue*, Package*);

Package* pkg_queue_pop(PackageQueue*);

Package* pkg_queue_front(PackageQueue* q);

int pkg_queue_isempty(PackageQueue*);

void pkg_clear_queue(PackageQueue * queue);


#endif
