// Copyright (C) 2010  Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#ifndef _RBTREE_H
#define _RBTREE_H 1

#include <dns/name.h>
#include <dns/rrset.h>
#include <dns/rrsetlist.h>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

using namespace isc::dns;
namespace isc {
namespace datasrc {

/// \brief Define rbtree color
enum RBTreeColor {BLACK = 1, RED};
class RBTree;

/// \brief \c RBNode class represents one domain name in the domain space

/// It has two roles, the first one is as one node in the \c RBTree,
/// the second one is to store the data related to DNS. As for the first role,
/// it has left, right, parent and color members
/// which is used to keep the balance of the \c RBTree. As for the second role,
//  it stores the RRsets that belong to the domain name and a rbtree which
/// includes all the subdomains of this node.
/// The name stored in the node is relative related to its parent node.
/// One special kind of node is non-terminal node
/// which has subdomains with RRset but itself doesn't have any RRsets.
///
/// \b Note: \c RBNode should be created or destroyed only by \c RBTree so
/// constructor and destructor function aren't exposed.

class RBNode : public boost::noncopyable {
public:
    /// only /c RBTree can create and destroy \c RBNode
    friend class RBTree;
    typedef boost::shared_ptr<RRsetList> RRsetListPtr;

    /// \name Test functions
    ///
    //@{
    /// \brief return whether current domain name has NS records
    bool isDelegate() const { return (is_delegate_); }

    /// \brief return whether current domain name is non-terminal
    /// A non-terminal domain has no rrsets but at least one of its descendant
    /// domain has rrset
    bool isNonterminal() const { return (is_nonterminal_); }

    /// \brief return the name of current node, it's relative to its parents
    //
    /// \todo Is it meaningful to return the absolute of the node?
    const Name &getName() const {return (name_); }

    // \brief return next node whose name is bigger than current node
    RBNode* successor();
    //@}


    /// \name modify function
    //@{

    /// \brief add the rrset to the node
    /// \Note: there is no check whether the node already has the rrset or not
    /// and no check about whether the name of the rrset is the same with the
    /// node or not.
    /// All of above is rely on interface user
    int addRRset(RRsetPtr rrset);
    //@}

private:
    /// \name Constructors and destructor
    //@{
    /// \param nullnode The null point for \c RBNode isnot use \c NULL, but
    /// use one specified
    /// default node singled as null node, this is intended to keep the code
    /// more unify
    RBNode(const Name& name, RRsetListPtr rrsets = RRsetListPtr(),
           RBNode* nullnode = NULL);

    /// the class isn't left to be inherited
    ~RBNode();
    //@}

    /// \brief copy the DNS related data to another node except the sub domain
    /// tree
    void cloneDNSData(RBNode& node);

    /// \brief when copy the DNS data from one node to another, except the
    /// RRsets, name etc,
    /// also needs to maintain the down and up relationship, which includes
    /// set the down point of current node and up point of sub domain tree
    void setDownTree(RBTree* down);

    /// data to maintain the rbtree balance
    RBNode*  parent_;
    RBNode*  left_;
    RBNode*  right_;
    int       color_;

    /// data to carry dns info
    Name      name_;
    RRsetListPtr rrsets_;
    RBTree*  down_;
    bool      is_delegate_;
    bool      is_nonterminal_;
};

/// \brief \c RBTree class represents all the domains with the same suffix,
/// so it can be used to store the domains in one zone.
///
/// \c RBTree is a generic red black tree, and contains all the nodes with
/// the same suffix, since each
/// name may have sub domain names so \c RBTree is a recursive data structure
/// or tree in tree.
/// So for one zone, several RBTrees may be involved. But from outside, the sub
/// tree is opaque for end users.
class RBTree : public boost::noncopyable {
    friend class RBNode;
public:
    /// \brief The return value for the \c find() method
    /// - EXACTMATCH: return the node in the tree exactly same with the target
    /// - FINDREFERRAL: return the node which is an ancestor of the target
    ///   containing NS record
    /// - NOTFOUND: other conditions except EXACTMATCH & FINDREFERRAL
    enum FindResult{EXACTMATCH, FINDREFERRAL, NOTFOUND};

    /// \name Constructor and Destructor
    //@{
    RBTree();

    /// \b Note: RBTree is not intended to be inherited so the destructor
    /// is not virtual
    ~RBTree();
    //@}

    /// \name Inquery methods
    //@{
    /// \brief Find the node with the name
    /// \param name Target to be found
    /// \param node Point to the node when the return vaule is \c not
    /// NOTFOUND, if the return value is NOTFOUND, the value of node is
    /// \c unknown
    FindResult find(const Name& name, RBNode** node) const;

    /// \brief Get the total node count in the tree
    int getNodeCount() const;
    //@}

    /// \name Debug function
    //@{
    /// \brief print the nodes in the trees
    /// \todo is it better to return one string instead of print to the stdout?
    void printTree(int depth = 0) const;
    //@}

    /// \name Modify function
    //@{
    /// \brief Insert the domain name into the tree
    /// \param name The name to be inserted into the tree
    /// \param inserted_node If no node with the name in the tree,
    /// new \c RBNode will be created, otherwise nothing will be done.
    /// Anyway the pointer point to the node with the name will be assigned to
    /// inserted_node
    /// \return return 0 means no node exists in the tree with the name before
    /// insert; return 1 means already has the node with the given name
    //
    /// To add an RRset into one node when it's not known whether the node
    /// already exists, it is better to call \c insert and then call the
    /// RBNode interface instead of calling \c find().
    int insert(const Name& name, RBNode** inserted_node);

    /// \brief Erase the node with the domain name
    /// \return If no node with the name, return 1; otherwise return 0
    int erase(const Name& name);
    //@}

private:
    /// \name RBTree balance functions
    //@{
    void deleteRebalance(RBNode* node);
    void insertRebalance(RBNode* node);
    RBNode* rightRotate(RBNode* p);
    RBNode* leftRotate(RBNode* p);
    //@}

    /// \name Helper functions
    //@{
    /// Each public function has related recursive helper function
    void eraseNode(RBNode* node);
    FindResult findHelper(const Name& name, RBTree** tree,
                          RBNode** node) const;
    int getNodeCountHelper(const RBNode* node) const;
    void printTreeHelper(RBNode* node, int depth) const;
    //@}

    RBNode*  root_;
    RBNode*  NULLNODE;
    RBNode*  up_;
    /// the node count of current tree except the sub domain trees
    unsigned int node_count_;
};
}
}

#endif  // _RBTREE_H

// Local Variables: 
// mode: c++
// End: 
