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

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Static Declarations
  -------------------------------------------------------------------------------*/
  static MsgFrag * merge( MsgFrag *a, MsgFrag *b );
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
  void sortFragments( MsgFrag **headPtr )
  {
    /*-------------------------------------------------
    Handle bad inputs or the end of the list
    -------------------------------------------------*/
    if ( !headPtr || !( *headPtr )->nextFragment )
    {
      return;
    }

    /*-------------------------------------------------
    Initialize the algorithm
    -------------------------------------------------*/
    MsgFrag *head = *headPtr;
    MsgFrag *a = nullptr;
    MsgFrag *b = nullptr;

    /*-------------------------------------------------
    Split current list into sublists
    -------------------------------------------------*/
    frontBackSplit( head, &a, &b );

    /*-------------------------------------------------
    Recursively sort the sublists
    -------------------------------------------------*/
    sortFragments( &a );
    sortFragments( &b );

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
  static MsgFrag * merge( MsgFrag *a, MsgFrag *b )
  {
    MsgFrag * result = nullptr;

    /*-------------------------------------------------
    Base case
    -------------------------------------------------*/
    if( a == nullptr )
    {
      return b;
    }
    else if( b == nullptr )
    {
      return a;
    }

    /*-------------------------------------------------
    Select A or B and recurse. By default packets are
    ordered from low to high.
    -------------------------------------------------*/
    if( a->fragmentNumber <= b->fragmentNumber )
    {
      result = a;
      result->nextFragment = merge( a->nextFragment, b );
    }
    else
    {
      result = b;
      result->nextFragment = merge( a, b->nextFragment );
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
    MsgFrag *slow = src;
    MsgFrag *fast = src->nextFragment;

    /*-------------------------------------------------
    Advance each node. Fast == 2x so it reaches the end
    first while the slow ptr is just about midline.
    -------------------------------------------------*/
    while( fast != nullptr )
    {
      fast = fast->nextFragment;
      if( fast != nullptr )
      {
        slow = slow->nextFragment;
        fast = fast->nextFragment;
      }
    }

    /*-------------------------------------------------
    Slow is behind the midpoint of the list, so split
    at that point.
    -------------------------------------------------*/
    *front = src;
    *back = slow->nextFragment;

    /*-------------------------------------------------
    Terminate the list to prevent further calls from
    expanding into the other half of the list.
    -------------------------------------------------*/
    slow->nextFragment = nullptr;
  }

}  // namespace Ripple
