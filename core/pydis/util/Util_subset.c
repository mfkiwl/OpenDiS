#include <stdlib.h>
#include "Home.h"

/* Functions from ParaDiS Util.c */

/*-------------------------------------------------------------------------
 *
 *      Function:     cross
 *      Description:  Calculates the cross product of two vector
 *                    supplied as arrays by the caller.
 *
 *      Arguments:
 *          a    3 element array containing components of the first vector
 *          b    3 element array containing components of the second vector
 *          c    3 element array in which to return to the caller the
 *               components of the cross product of vectors <a> and <b>.
 *
 *------------------------------------------------------------------------*/
void cross(real8 a[3], real8 b[3], real8 c[3])
{
    c[0]=a[1]*b[2]-a[2]*b[1];
    c[1]=a[2]*b[0]-a[0]*b[2];
    c[2]=a[0]*b[1]-a[1]*b[0];
}

/*-------------------------------------------------------------------------
 *
 *      Function:     Normalize
 *      Description:  Normalize vector a
 *
 *      Arguments:
 *          ax, ay, az  Pointers to the three elements of vector a.
 *                      The normalized values will be returned to the
 *                      caller via these same pointers.
 *
 *------------------------------------------------------------------------*/
void Normalize(real8 *ax, real8 *ay, real8 *az)
{
        real8 a2, a;

        a2 = ((*ax)*(*ax) + (*ay)*(*ay) + (*az)*(*az));

        if (a2 > 0.0) {
            a = sqrt(a2);
            *ax /= a;
            *ay /= a;
            *az /= a;
        }

        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     NormalizeVec
 *      Description:  Normalize vector a
 *
 *      Arguments:
 *          vec   Three-element vector to be normalized.  Contents
 *                are updated before control returned to the caller.
 *
 *------------------------------------------------------------------------*/
void NormalizeVec(real8 vec[3])
{
        real8 a2, a;

        a2 = (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);

        if (a2 > 0.0) {
            a = sqrt(a2);
            vec[0] /= a;
            vec[1] /= a;
            vec[2] /= a;
        }

        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     Connected
 *      Description:  Determines whether or not two nodes are connected
 *                    by a dislocation segment.
 *
 *      Arguments:
 *          node1     Pointer to first node
 *          node2     Pointer to second node
 *          armID     Pointer to location in which to return to the
 *                    caller the index (for node1) of the arm connecting
 *                    the two nodes.  (If the nodes are not connected
 *                    the contents of this pointer will be set to -1)
 *
 *      Returns:   0 if the nodes are not connected.
 *                 1 if the nodes are connected.
 *
 *------------------------------------------------------------------------*/
int Connected(Node_t *node1, Node_t *node2, int *armID)
{
        int i, dom2, idx2;

        *armID = -1;

        if ((node1 == NULL) || (node2 == NULL)) {
            return(0);
        }
    
        dom2 = node2->myTag.domainID;
        idx2 = node2->myTag.index;    

        for (i = 0; i < node1->numNbrs; i++) {
            if ((dom2 == node1->nbrTag[i].domainID) &&
                (idx2==node1->nbrTag[i].index)) {
                *armID = i;
                return(1);
            }
        }

        return 0;
}

/*---------------------------------------------------------------------------
 *
 *      Function:       DomainOwnsSeg
 *      Description:    Determines if the specified domain owns
 *                      the segment beginning in the current domain
 *                      and terminating at the domain indicated by
 *                      <endTag>.
 *
 *                      Note:  Ownership of segments crossing
 *                             domain boundaries alternates on
 *                             and even/odd cycles.  Additionally,
 *                             the ownership is dependent on the
 *                             class of topological operation being
 *                             considered (i.e. during remesh operations
 *                             the ownership is the exact reverse of
 *                             ownership during collision handling.)
 *
 *      Arguments:
 *              opClass         Class of topological operation during which
 *                              this function is being invoked.  Valid
 *                              values are:
 *                                  OPCLASS_SEPARATION
 *                                  OPCLASS_COLLISION
 *                                  OPCLASS_REMESH
 *              thisDomain      domain containing the first endpoint of
 *                              the segment in question.
 *              endTag          pointer to the tag of the second endpoint
 *                              of the segment.
 *
 *      Returns:  1 if <thisDomain> owns the segment
 *                0 in all other cases.
 *
 *-------------------------------------------------------------------------*/
int DomainOwnsSeg(Home_t *home, int opClass, int thisDomain, Tag_t *endTag)
{
        int ownsSeg = 1;

/*
 *      If both endpoints are in the same domain, the domain owns
 *      the segment.
 */
        if (thisDomain == endTag->domainID) {
            return(1);
        }

/*
 *      For collision handling and node separations, ownership
 *      of segments crossing domain boundaries is the lower
 *      numbered domain on even numbered cycles and the higher
 *      numbered domain for odd numbered cycles.
 *
 *      For remesh operations, ownership rules are the opposite
 *      of those used for collision handling.
 */
        switch (opClass) {
        case OPCLASS_SEPARATION:
        case OPCLASS_COLLISION:
            if (home->cycle & 0x01)
                ownsSeg = (thisDomain > endTag->domainID);
            else
                ownsSeg = (thisDomain < endTag->domainID);
            break;
        case OPCLASS_REMESH:
            if (home->cycle & 0x01)
                ownsSeg = (thisDomain < endTag->domainID);
            else
                ownsSeg = (thisDomain > endTag->domainID);
            break;
        default:
            Fatal("Invalid opClass %d in DomainOwnsSeg()", opClass);
            break;
        }

        return(ownsSeg);
}

/*-------------------------------------------------------------------------
 *
 *      Function:     Fatal
 *      Description:  Prints a user specified message, aborts all
 *                    other parallel tasks (if any) and self-terminates
 *
 *      Arguments:    This function accepts a variable number of arguments
 *                    in the same fashion as printf(), with the first
 *                    option being the format string which determines
 *                    how the remainder of the arguments are to be
 *                    interpreted.
 *
 *------------------------------------------------------------------------*/
void Fatal(char *format, ...) 
{
        char    msg[512];
        va_list args;

        va_start(args, format);
        vsnprintf(msg, sizeof(msg)-1, format, args);
        msg[sizeof(msg)-1] = 0;
        va_end(args);
        printf("Fatal: %s\n", msg);

#ifdef PARALLEL
        MPI_Abort(MPI_COMM_WORLD, -1);
#endif
        exit(1);
}

/*-------------------------------------------------------------------------
 *
 *      Function:     FoldBox
 *      Description:  Given a set of coordinates, adjusts the 
 *                    coordinates to the corresponding point within
 *                    the primary (non-periodic) image of the problem
 *                    space.  If the provided coordinates are already
 *                    within the primary image, no adjustments are
 *                    made.
 *
 *      Arguments:
 *          param       Pointer to structure containing control parameters
 *          x, y, z     Pointers to components of coordinate.  On return
 *                      to the caller, these coordinates will have been
 *                      adjusted (if necessary) to the coordinates of the
 *                      corresponding point within the primary image of
 *                      the problem space.
 *
 *------------------------------------------------------------------------*/
void FoldBox(Param_t *param, real8 *x, real8 *y, real8 *z)
{
        real8 xc, yc, zc;

        xc = (param->maxSideX + param->minSideX) * 0.5;
        yc = (param->maxSideY + param->minSideY) * 0.5;
        zc = (param->maxSideZ + param->minSideZ) * 0.5;
    
        if (param->xBoundType == Periodic) {
            *x -= rint((*x-xc)*param->invLx) * param->Lx;
        }

        if (param->yBoundType == Periodic) {
            *y -= rint((*y-yc)*param->invLy) * param->Ly;
        }

        if (param->zBoundType == Periodic) {
            *z -= rint((*z-zc)*param->invLz) * param->Lz;
        }
    
        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:    GetArmID
 *      Description: Given two node pointers, return arm ID for node1
 *                   if node2 is its neighbor.
 *
 *      Arguments:
 *          node1    Pointer to first node structure
 *          node2    Pointer to second node structure
 *
 *      Returns:  Non-negative index of the arm of node1 terminating
 *                at node2 if the nodes are connected.
 *                -1 if the two nodes are not connected.
 *                
 *------------------------------------------------------------------------*/
int GetArmID (Home_t *home, Node_t *node1, Node_t *node2)
{
        int i, domID, index;

        if ((node1 == NULL) || (node2 == NULL)) {
            return(-1);
        }
    
        domID = node2->myTag.domainID;
        index = node2->myTag.index;

        for(i = 0; i < node1->numNbrs; i++) {
            if ((domID == node1->nbrTag[i].domainID) &&
                (index == node1->nbrTag[i].index)) {
                return(i);
            }
        }

        return(-1);
}

/*-------------------------------------------------------------------------
 *
 *      Function:     GetNeighborNode
 *      Description:  Given a node pointer, return the pointer to
 *                    the n'th neighbor of the node.
 *
 *      Arguments:
 *          node    Pointer to a node.
 *          n       Index in the node's neighbor list of the 
 *                  neighbor to return to the caller.
 *
 *      Returns:    Pointer to the requested neighbor if found.
 *                  NULL in all other cases.
 *
 * FIX ME!  Check if this function will ever be called with sparse arm list 
 *
 *------------------------------------------------------------------------*/
Node_t *GetNeighborNode(Home_t *home, Node_t *node, int n)
{
        int    i, j;
        Node_t *neighbor;
#if 0
/*
 *      New version which assumes no invalid arms in arm list
 */
        if (n >= node->numNbrs) {
            printf("GetNeighborNode: Error finding neighbor %d\n", n);
            PrintNode(node);
            return((Node_t *)NULL);
        }

        neighbor = GetNodeFromTag(home, node->nbrTag[n]);

        return(neighbor);
#else
/*
 *      Old version which assumes the arm list may be sparsely
 *      populated and returns the n'th valid neighbor, which may
 *      not be at index n.  
 */
        j = -1;
 
        for (i = 0; i < node->numNbrs; i++) {

            if (node->nbrTag[i].domainID >= 0) j++;

            if (j == n) {
                neighbor = GetNodeFromTag(home, node->nbrTag[i]);
                return(neighbor);
            }
        }

        printf("GetNeighborNode returning NULL for node (%d,%d) nbr %d\n",
               node->myTag.domainID, node->myTag.index, n);
        PrintNode(node);

        return((Node_t *)NULL);
#endif
}

/*-------------------------------------------------------------------------
 *
 *      Function:     GetNodeFromTag
 *      Description:  Given a node tag, returns a pointer to the
 *                    corresponding node structure.
 *
 *                    NOTE: If the specified tag is for a local node
 *                    and the local node is not found, the function
 *                    will force a code abort with a fatal error.
 *
 *      Arguments:
 *          tag    Tag identifying the desired node
 *
 *      Returns:    Pointer to the requested node if found.
 *                  NULL in all other cases.
 *
 *------------------------------------------------------------------------*/
Node_t *GetNodeFromTag (Home_t *home, Tag_t tag)
{
        Node_t         *node;
        RemoteDomain_t *remDom;
   
        if (tag.domainID < 0 || tag.index < 0) {
            Fatal("GetNodeFromTag: invalid tag (%d,%d)",
                  tag.domainID, tag.index);
        }

/*
 *      If the tag is for a local domain, look up the node in
 *      the local <nodeKeys> array.
 */
        if (tag.domainID == home->myDomain) {

            if (tag.index >= home->newNodeKeyPtr) {
                return((Node_t *)NULL);
            }

            if ((node = home->nodeKeys[tag.index]) == (Node_t *)NULL) {
                return((Node_t *)NULL);
            }

            return(node);

        } else {
/*
 *          If the node is in a remote domain, there are valid situations
 *          in which the current domain does not have information on
 *          either the remote doamin or the remote node.  Hence, it
 *          is not an error to return a NULL pointer.
 */
            remDom = home->remoteDomainKeys[tag.domainID];

            if (remDom == NULL) {
                return((Node_t *)NULL);
            }

            if (tag.index >= remDom->maxTagIndex) {
                return((Node_t *)NULL);
            }
      
            node = remDom->nodeKeys[tag.index];
            return(node);
        }
}

/*-------------------------------------------------------------------------
 *
 *      Function:     ResetSegForces
 *      Description:  Reset the segment forces on nodeA for the
 *                    segment terminating at nodeB, then re-sum the
 *                    total nodal forces for nodeA
 *
 *      Arguments:
 *          nodeA       pointer to node for which to reset seg forces
 *          nodeBtag    pointer to the tag of the node at which the
 *                      segment in question terminates
 *          fx,fy,fz    segment forces 
 *          globalOp    set to 1 if this operation is an operation
 *                      that will have to be passed on to remote
 *                      domains
 *
 *------------------------------------------------------------------------*/
void ResetSegForces(Home_t *home, Node_t *nodeA, Tag_t *nodeBtag,
                    real8 fx, real8 fy, real8 fz, int globalOp)
{
        int i;
    
/*
 *      If other domains need to be notified of this operation, add the
 *      action to the operation list
 */
        if (globalOp) {
            AddOp(home, RESET_SEG_FORCES,
                nodeA->myTag.domainID,
                nodeA->myTag.index,
                nodeBtag->domainID,
                nodeBtag->index,
                -1,-1,
                0.0, 0.0, 0.0, /* bx, by, bz */
                fx, fy, fz,
                0.0,0.0,0.0);  /* nx, ny, nz */
        }
    
/*
 *      Locate the segment of nodeA terminating at nodeb and update
 *      forces for that segment.
 */
        for (i = 0; i < nodeA->numNbrs; i++) {
            if ((nodeA->nbrTag[i].domainID == nodeBtag->domainID) &&
                (nodeA->nbrTag[i].index == nodeBtag->index)) {
                nodeA->armfx[i] = fx;
                nodeA->armfy[i] = fy;
                nodeA->armfz[i] = fz;
                break;
            }
        }
    
/*
 *      Reset the total forces for nodeA based on all its segment forces
 */
        nodeA->fX = 0;
        nodeA->fY = 0;
        nodeA->fZ = 0;
            
        for (i = 0; i < nodeA->numNbrs; i++) {
            nodeA->fX += nodeA->armfx[i];
            nodeA->fY += nodeA->armfy[i];
            nodeA->fZ += nodeA->armfz[i];
        }
    
        nodeA->flags |= NODE_RESET_FORCES;
    
        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     MarkNodeForceObsolete
 *      Description:  Sets a flag for the specified node indicating that
 *                    the nodal force and velocity values are obsolete and
 *                    must be recalculated.  If the node is not local to
 *                    the current domain, the operation list going to
 *                    the remote domains will be modified to include a
 *                    operation to force the owning domain to recalculate
 *                    the node's force and velocity.
 *
 *      Arguments:
 *          node    pointer to node for which the force and velocity
 *                  values must be recomputed
 *
 *------------------------------------------------------------------------*/
void MarkNodeForceObsolete(Home_t *home, Node_t *node)
{
        node->flags |= NODE_RESET_FORCES;

/*
 *      If the node is locally owned, we're done.  If not, we
 *      need to let the owning domain know it needs to recompute
 *      the force/velocity.
 */
        if (node->myTag.domainID == home->myDomain) return;

        AddOp(home, MARK_FORCES_OBSOLETE,
            node->myTag.domainID,
            node->myTag.index,
            -1, -1,
            -1, -1,
            0.0, 0.0, 0.0, /* bx, by, bz */
            0.0, 0.0, 0.0, /* vx, vy, vz */
            0.0,0.0,0.0);  /* nx, ny, nz */

        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     PBCPOSITION
 *      Description:  Finds the position of the image of point (x,y,z)
 *                    that is closest to point (x0,y0,z0).
 *
 *                    NOTE: The returned position is not required
 *                    to be in the primary image but may actually be in
 *                    one of the periodic images
 *                  
 *      Arguments:
 *          param       Pointer to structure containing control parameters
 *          x0, y0, z0  Components of position used as a base.
 *          x, y, z     Pointers to components of secondary point.  These
 *                      values will be overwritten with coordinates of the
 *                      image of this point closest to (x0,y0,z0).
 *
 *------------------------------------------------------------------------*/
void PBCPOSITION(Param_t *param, real8 x0, real8 y0, real8 z0,
                 real8 *x, real8 *y, real8 *z)
{
/*
 *      If periodic boundaries are not in use, the provided position
 *      of (x,y,z) will not be adjusted since there are no other
 *      images available.
 */
        if (param->xBoundType == Periodic) { 
            *x -= rint((*x-x0)*param->invLx) * param->Lx;
        }

        if (param->yBoundType == Periodic) {
            *y -= rint((*y-y0)*param->invLy) * param->Ly;
        }

        if (param->zBoundType == Periodic) {
            *z -= rint((*z-z0)*param->invLz) * param->Lz;
        }
    
        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     ZImage
 *      Description:  Finds the minimum image of (x,y,z) in the
 *                    problem box.
 *
 *                    Typical use of this function specifies (x,y,z)
 *                    as vector from a source point to a secondary
 *                    point.  Upon return to the caller, the vector
 *                    will have been adjusted to be a vector from the
 *                    source point to the closest image of the secondary
 *                    point.
 *                  
 *      Arguments:
 *          param       Pointer to structure containing control parameters
 *          x, y, z     Pointers to components of point (or vector).
 *
 *------------------------------------------------------------------------*/
void ZImage(Param_t *param, real8 *x, real8 *y, real8 *z)
{
/*
 *      If periodic boundaries are not in use, the provided position
 *      of (x,y,z) will not be adjusted since there are no other
 *      images available.
 */
        if (param->xBoundType == Periodic) {
            *x -= rint(*x * param->invLx) * param->Lx;
        }

        if (param->yBoundType == Periodic) {
            *y -= rint(*y * param->invLy) * param->Ly;
        }

        if (param->zBoundType == Periodic) {
            *z -= rint(*z * param->invLz) * param->Lz;
        }
    
        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     PrintNode
 *      Description:  For the specified node, print some interesting
 *                    items of data.
 *
 *------------------------------------------------------------------------*/
void PrintNode(Node_t *node)
{
        int i;

        if (node == (Node_t *)NULL) return;

#if 1
        printf("  node(%d,%d) arms %d, ",
               node->myTag.domainID, node->myTag.index, 
               node->numNbrs);
#if 1
        for (i = 0; i < node->numNbrs; i++) {
            printf("(%d,%d) ", node->nbrTag[i].domainID,
                   node->nbrTag[i].index);
        }
#endif
        printf("\n");
#endif

#if 1
/*
 *      Print the nodal position
 */
        printf("  node(%d,%d) position = (%.15e %.15e %.15e)\n",
               node->myTag.domainID, node->myTag.index, 
               node->x, node->y, node->z);
#endif

#if 1
/*
 *      Print the nodal velocity and total node force
 */
        printf("  node(%d,%d) v = (%.15e %.15e %.15e)\n",
               node->myTag.domainID, node->myTag.index, 
               node->vX, node->vY, node->vZ);
        printf("  node(%d,%d) f = (%.15e %.15e %.15e)\n",
               node->myTag.domainID, node->myTag.index, 
               node->fX, node->fY, node->fZ);
#endif

#if 1
/*
 *      Print the arm specific forces
 */
        for (i = 0; i < node->numNbrs; i++) {
            printf("  node(%d,%d) arm[%d]-> (%d %d) f = (%.15e %.15e %.15e)\n",
                   node->myTag.domainID, node->myTag.index, i,       
                   node->nbrTag[i].domainID, node->nbrTag[i].index,
                   node->armfx[i], node->armfy[i], node->armfz[i]);
        }
#endif

#if 1
/*
 *      Print the burger's vector for each arm of the node
 */
        for (i = 0; i < node->numNbrs; i++) {
            printf("  node(%d,%d) arm[%d]-> (%d %d) b = (%.15e %.15e %.15e)\n",
                   node->myTag.domainID, node->myTag.index, i,       
                   node->nbrTag[i].domainID, node->nbrTag[i].index,
                   node->burgX[i], node->burgY[i], node->burgZ[i]);
        }
#endif

#if 1
/*
 *      Print the glide plane normal for each arm of the node
 */
        for (i = 0; i < node->numNbrs; i++) {
            printf("  node(%d,%d) arm[%d]-> (%d %d) n = (%.15e %.15e %.15e)\n",
                   node->myTag.domainID,node->myTag.index, i,       
                   node->nbrTag[i].domainID,node->nbrTag[i].index,
                   node->nx[i],node->ny[i],node->nz[i]);
        }
#endif

        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:     RecalcSegGlidePlane
 *      Description:  Calculate and reset (if necessary) the glide plane
 *                    normal for a segment.  This function will only update
 *                    the information locally.  It is assumed the information
 *                    will be passed to remote domains elsewhere as needed.
 *
 *      Arguments:
 *          node1, node2   pointers to nodal endpoints of the segment
 *          ignoreIfScrew  Toggle.  If set to 1, the function will
 *                         leave the segment glide plane as is if the
 *                         segment is screw.  Otherwise it will pick
 *                         an appropriate glide plane for the bugers vector
 *
 *------------------------------------------------------------------------*/
void RecalcSegGlidePlane(Home_t *home, Node_t *node1, Node_t *node2,
                         int ignoreIfScrew)
{
        int node1SegID, node2SegID;
        real8 burg[3], lineDir[3], newPlane[3];

/*
 *      Just a couple quick sanity checks.
 */
        if ((node1 == (Node_t *)NULL) ||
            (node2 == (Node_t *)NULL) ||
            (node1 == node2)) {
            return;
        }

/*
 *      It is possible that the two nodes are not really connected.
 *      This can happen in cases such as MeshCoarsen() where a node is
 *      removed leaving two nodes doubly linked, and when the double links
 *      get reconciled they annihilate each other, leaving the two nodes
 *      unconnected.  In situations like that, there's nothing for this
 *      function to do...
 */
        if (!Connected(node1, node2, &node1SegID)) {
            return;
        }

        node2SegID = GetArmID(home, node2, node1);

        burg[X] = node1->burgX[node1SegID];
        burg[Y] = node1->burgY[node1SegID];
        burg[Z] = node1->burgZ[node1SegID];

        lineDir[X] = node2->x - node1->x;
        lineDir[Y] = node2->y - node1->y;
        lineDir[Z] = node2->z - node1->z;

        ZImage(home->param, &lineDir[X], &lineDir[Y], &lineDir[Z]);
        NormalizeVec(lineDir);

        FindPreciseGlidePlane(home, burg, lineDir, newPlane);

        if (DotProduct(newPlane, newPlane) < 1.0e-03) {
            if (ignoreIfScrew) return;
            PickScrewGlidePlane(home, burg, newPlane);
        }

        Normalize(&newPlane[X], &newPlane[Y], &newPlane[Z]);

        node1->nx[node1SegID] = newPlane[X];
        node1->ny[node1SegID] = newPlane[Y];
        node1->nz[node1SegID] = newPlane[Z];

        node2->nx[node2SegID] = newPlane[X];
        node2->ny[node2SegID] = newPlane[Y];
        node2->nz[node2SegID] = newPlane[Z];

        return;
}

/*-------------------------------------------------------------------------
 *
 *      Function:      AddOp
 *      Description:   Add a topological operation to the list to
 *                     be sent to remote domains for processing.
 *
 *      Arguments:
 *          type        Type of operation to be performed
 *          dom1, idx1  Tag information for the 1st node involved
 *                      in the operation.
 *          dom2, idx2  Tag information for the 2nd node involved
 *                      in the operation.  (If no second node is
 *                      involved in this operation, these values
 *                      should be set to -1)
 *          dom3, idx3  Tag information for the 3rd node involved
 *                      in the operation.  (If no third node is
 *                      involved in this operation, these values
 *                      should be set to -1)
 *          bx, by, bz  Components of the burgers vector for the
 *                      operation.  (If not applicable to the
 *                      operation, these should be zeroes)
 *          x, y, z     Components of node's position (If not
 *                      applicable to the operation, should be zero'ed)
 *          nx, ny, nz  Components of glide plane normal. (If not
 *                      applicable to the operation, should be zero'ed)
 *          
 *------------------------------------------------------------------------*/
void AddOp (Home_t *home,
            OpType_t type, int dom1, int idx1,
            int dom2, int idx2, int dom3, int idx3,
            real8 bx, real8 by, real8 bz,
            real8 x, real8 y, real8 z,
            real8 nx, real8 ny, real8 nz) 
{
        Operate_t *op;

/*
 *      Make sure the buffer allocated for the operation list is
 *      large enough to contain another operation.  If not, increase
 *      the size of the buffer.
 */
        if (home->OpCount >= home->OpListLen) {
            ExtendOpList (home);
        }
        
        op = &home->opList[home->OpCount];

        op->type = type;
        op->dom1 = dom1;
        op->idx1 = idx1;
        op->dom2 = dom2;
        op->idx2 = idx2;
        op->dom3 = dom3;
        op->idx3 = idx3;
        op->bx = bx;
        op->by = by;
        op->bz = bz;
        op->x = x;
        op->y = y;
        op->z = z;
        op->nx = nx;
        op->ny = ny;
        op->nz = nz;
        home->OpCount++;

        return;
}


/*-------------------------------------------------------------------------
 *
 *      Function:      ClearOpList
 *      Description:   Zero's both the list of operations which is
 *                     communicated to remote domains during various
 *                     stages of topological changes and the count
 *                     of operations on the list .
 *
 *------------------------------------------------------------------------*/
void ClearOpList (Home_t *home)
{
        memset(home->opList, 0, sizeof(Operate_t)*home->OpListLen);
        home->OpCount = 0;

        return;
}


/*-------------------------------------------------------------------------
 *
 *      Function:      ExtendOpList
 *      Description:   Increases the buffer size dedicated to the
 *                     list of operations sent to remote domains
 *                     after topological changes.
 *
 *------------------------------------------------------------------------*/
void ExtendOpList (Home_t *home)
{
        int newSize;

        newSize = (home->OpListLen+OpBlock_Count) * sizeof(Operate_t);
        home->opList = (Operate_t *) realloc(home->opList, newSize);
        home->OpListLen += OpBlock_Count;

        return;
}


/*-------------------------------------------------------------------------
 *
 *      Function:      FreeOpList
 *      Description:   Deallocates the memory dedicated to the operation
 *                     list sent to remote domains after topological
 *                     changes.
 *
 *                     NOTE:  The current code no longer frees the
 *                     buffer associated with the list, instead allowing
 *                     it to persist through the entire execution for
 *                     efficiency reasons.  Hence, this function is
 *                     currently obsolete.
 *
 *------------------------------------------------------------------------*/
void FreeOpList (Home_t *home)
{
        free(home->opList);
        home->OpCount = 0;
        home->OpListLen = 0;
}


/*-------------------------------------------------------------------------
 *
 *      Function:      InitOpList
 *      Description:   Allocates an initial block of memory for the
 *                     list of operations this domain will send to
 *                     remote domains for processing during the
 *                     various stages of topological changes.  
 *
 *                     This function should only need to be called 
 *                     one time during initialization of the application.
 *                     Adding operations to the list as well as altering
 *                     the list size will be handled dynamically as
 *                     necessary
 *
 *------------------------------------------------------------------------*/
void InitOpList (Home_t *home)
{
        home->opList = (Operate_t *) malloc(sizeof(Operate_t)*OpBlock_Count);
        home->OpCount = 0;
        home->OpListLen = OpBlock_Count;
        home->rcvOpCount = 0;
        home->rcvOpList = 0;

        return;
}
