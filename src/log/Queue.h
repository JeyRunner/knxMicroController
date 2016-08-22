//
// Created by joshua on 08.08.16.
//

#ifndef KNX_QUEUE_H
#define KNX_QUEUE_H


#include "Log.h"

template <class T>
class Queue : protected Log
{
    public:
        Queue();
        void init();
        void push(T element);   // insert new element
        T pop();                // get next element and remove it
        
        int size();             // amount of elements
    
        struct QueueElement
        {
            T contend;
            QueueElement *next;
        };
        QueueElement* head;
        volatile int size_;
    
    private:
        QueueElement* end;
};

template class Queue<char *>;
template class Queue<int>;
template class Queue<char>;
#endif //KNX_QUEUE_H
