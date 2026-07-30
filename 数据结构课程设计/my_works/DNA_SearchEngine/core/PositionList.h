#ifndef POSITIONLIST_H
#define POSITIONLIST_H

template <class T>
struct ListNode
{
    T _data;
    ListNode<T> *_next;
};

// 每个 K-mer 的全部出现位置由手写单链表保存。
using PositionList = ListNode<int> *;

#endif // POSITIONLIST_H
