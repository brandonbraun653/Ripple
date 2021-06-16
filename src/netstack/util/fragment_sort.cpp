/********************************************************************************
 *  File Name:
 *    fragment_sort.cpp
 *
 *  Description:
 *    Implementation of message fragment sorting algorithm
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/netstack>

namespace Ripple::TMPFragment
{
  /*-------------------------------------------------------------------------------
  Static Declarations
  -------------------------------------------------------------------------------*/
  static MsgFrag *merge( MsgFrag *a, MsgFrag *b );
  static void frontBackSplit( MsgFrag *src, MsgFrag **front, MsgFrag **back );

  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  /**
   * @brief Sorts a list of message fragments, ordered by packet number.
   * Algorithm uses merge-sort, referenced from GeeksForGeeks.
   *
   * @param headPtr       Start of the fragment list
   */
  void sort( MsgFrag **headPtr )
  {
    /*-------------------------------------------------
    Handle bad inputs or the end of the list
    -------------------------------------------------*/
    if ( !headPtr || !( *headPtr )->next )
    {
      return;
    }

    /*-------------------------------------------------
    Initialize the algorithm
    -------------------------------------------------*/
    MsgFrag *head = *headPtr;
    MsgFrag *a    = nullptr;
    MsgFrag *b    = nullptr;

    /*-------------------------------------------------
    Split current list into sublists
    -------------------------------------------------*/
    frontBackSplit( head, &a, &b );

    /*-------------------------------------------------
    Recursively sort the sublists
    -------------------------------------------------*/
    sort( &a );
    sort( &b );

    /*-------------------------------------------------
    Merge the sorted lists together
    -------------------------------------------------*/
    *headPtr = merge( a, b );
  }


  /**
   * @brief Recursively merges two message fragment lists
   *
   * @param a           First list
   * @param b           Second list
   * @return MsgFrag*   Resulting merged list
   */
  static MsgFrag *merge( MsgFrag *a, MsgFrag *b )
  {
    MsgFrag *result = nullptr;

    /*-------------------------------------------------
    Base case
    -------------------------------------------------*/
    if ( a == nullptr )
    {
      return b;
    }
    else if ( b == nullptr )
    {
      return a;
    }

    /*-------------------------------------------------
    Select A or B and recurse. By default packets are
    ordered from low to high.
    -------------------------------------------------*/
    if ( a->fragmentNumber <= b->fragmentNumber )
    {
      result       = a;
      result->next = merge( a->next, b );
    }
    else
    {
      result       = b;
      result->next = merge( a, b->next );
    }

    return result;
  }


  /**
   * @brief Splits a linked list into two sublists, split roughly in the middle.
   *
   * @param src         Starting list
   * @param front       Front half of the list
   * @param back        Second half of the list
   */
  static void frontBackSplit( MsgFrag *src, MsgFrag **front, MsgFrag **back )
  {
    MsgFrag *slow      = src;
    MsgFrag *fast      = src->next;
    MsgFrag *slow_prev = nullptr;

    /*-------------------------------------------------
    Input Protection
    -------------------------------------------------*/
    if ( !src || !front || !back )
    {
      return;
    }

    /*-------------------------------------------------
    Special cases where only 1 or 2 items in the list
    -------------------------------------------------*/
    if ( src->next == nullptr )
    {
      *front = src;
      *back  = nullptr;

      return;
    }
    else if ( src->next->next == nullptr )
    {
      *front = src;
      *back  = src->next;

      /* Terminate the first list */
      ( *front )->next = nullptr;

      return;
    }

    /*-------------------------------------------------
    Three or more items. Use the fast/slow ptr approach
    -------------------------------------------------*/
    while ( fast && fast->next != nullptr )
    {
      fast = fast->next;

      if ( fast )
      {
        slow = slow->next;
        fast = fast->next;
      }
    }

    /*-------------------------------------------------
    Slow is behind the midpoint of the list, so split
    at that point.
    -------------------------------------------------*/
    *front = src;
    *back  = slow->next;

    /*-------------------------------------------------
    Terminate the list to prevent further calls from
    expanding into the other half of the list.
    -------------------------------------------------*/
    slow->next = nullptr;
  }

}    // namespace Ripple::Fragment
