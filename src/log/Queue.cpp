//
// Created by joshua on 08.08.16.
//

#include <stdlib.h>
#include "Queue.h"


template <class  T>
Queue<T>::Queue()
{
    head = nullptr;
    end  = nullptr;
    size_=0;
}

template <class  T>
void Queue<T>::init()
{
    head = nullptr;
    end  = nullptr;
    size_=0;
}


template <class  T>
void Queue<T>::push(T element)
{
    QueueElement* queueElement = (QueueElement*)malloc(sizeof(QueueElement));
    queueElement->contend = element;
    //info("push new element '%s' '%d'", element, element);
    
    // if empty
    if (head == nullptr)
    {
        //info("head = null");
        head = queueElement;
    }
    
    // not if empty
    if (end != nullptr)
    {
        end->next = queueElement;
    }
    
    
    end = queueElement;
    size_++;
}


template <class  T>
T Queue<T>::pop()
{
    if (head == nullptr)
        return 0;
    
    //info("pop element '%s' '%d'", head->contend, head->contend);
    
    T contend = head->contend;
    QueueElement* next = head->next;
    free(&head);
    head = next;
    
    size_--;
    //size_v--;
    return contend;
}

template <class  T>
int Queue<T>::size()
{
    return size_;
}
  