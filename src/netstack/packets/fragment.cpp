/********************************************************************************
 *  File Name:
 *    fragment.cpp
 *
 *  Description:
 *    Fragment methods
 *
 *  2021 | Brandon Braun | brandonbraun653@gmail.com
 *******************************************************************************/

/* Ripple Includes */
#include <Ripple/netstack>

namespace Ripple
{
  /*-------------------------------------------------------------------------------
  Static Functions
  -------------------------------------------------------------------------------*/
  /**
   * @brief Recursively merges two message fragment lists
   *
   * @param a               First list
   * @param b               Second list
   * @return Fragment_sPtr  Resulting merged list
   */
  static Fragment_sPtr merge( Fragment_sPtr &a, Fragment_sPtr &b )
  {
    Fragment_sPtr result;

    /*-------------------------------------------------
    Base case
    -------------------------------------------------*/
    if ( !a )
    {
      return b;
    }
    else if ( !b )
    {
      return a;
    }

    /*-------------------------------------------------
    Select A or B and recurse. By default packets are
    ordered from low to high.
    -------------------------------------------------*/
    if ( a->number <= b->number )
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
  static void frontBackSplit( Fragment_sPtr src, Fragment_sPtr *front, Fragment_sPtr *back )
  {
    Fragment_sPtr slow = src;
    Fragment_sPtr fast = src->next;
    Fragment_sPtr slow_prev;

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
    if ( !src->next )
    {
      *front = src;
      *back  = Fragment_sPtr();

      return;
    }
    else if ( !src->next->next )
    {
      *front = src;
      *back  = src->next;

      /* Terminate the first list */
      ( *front )->next = Fragment_sPtr();

      return;
    }

    /*-------------------------------------------------
    Three or more items. Use the fast/slow ptr approach
    -------------------------------------------------*/
    while ( fast && fast->next )
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
    slow->next = Fragment_sPtr();
  }


  /*-------------------------------------------------------------------------------
  Public Functions
  -------------------------------------------------------------------------------*/
  Fragment_sPtr allocFragment( INetMgr *const context, const size_t payload_bytes )
  {
    Fragment_sPtr local( context );
    local->data = RefPtr<void *>( context, payload_bytes );

    return local;
  }


  /**
   * @brief Sorts a list of message fragments, ordered by packet number.
   * Algorithm uses merge-sort, referenced from GeeksForGeeks.
   *
   * @param headPtr       Start of the fragment list
   */
  void fragmentSort( Fragment_sPtr *headPtr )
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
    Fragment_sPtr head = *headPtr;
    Fragment_sPtr a    = Fragment_sPtr();
    Fragment_sPtr b    = Fragment_sPtr();

    /*-------------------------------------------------
    Split current list into sublists
    -------------------------------------------------*/
    frontBackSplit( head, &a, &b );

    /*-------------------------------------------------
    Recursively sort the sublists
    -------------------------------------------------*/
    fragmentSort( &a );
    fragmentSort( &b );

    /*-------------------------------------------------
    Merge the sorted lists together
    -------------------------------------------------*/
    *headPtr = merge( a, b );
  }


  /**
   * @brief Copies a fragment into freshly allocated memory.
   *
   * @param context       Memory context
   * @param fragment      Fragment being copied
   * @return Fragment_sPtr
   */
  Fragment_sPtr fragmentShallowCopy( INetMgr *const context, const Fragment_sPtr &fragment )
  {
    /*-------------------------------------------------
    Allocate a new fragment
    -------------------------------------------------*/
    auto newFrag = allocFragment( context, fragment->length );

    /*-------------------------------------------------
    Copy over the old fragment data, but discard links
    -------------------------------------------------*/
    newFrag->next   = Fragment_sPtr();
    newFrag->length = fragment->length;
    newFrag->number = fragment->number;
    newFrag->uuid   = fragment->uuid;

    void **dst = newFrag->data.get();
    void **src = fragment->data.get();
    memcpy( *dst, *src, fragment->length );

    return newFrag;
  }
}    // namespace Ripple
