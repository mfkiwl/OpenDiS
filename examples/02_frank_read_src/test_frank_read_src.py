import numpy as np
import sys, os

pydis_paths = [os.path.abspath('../../python'),os.path.abspath('../../lib'),os.path.abspath('../../core/pydis/python')]
[sys.path.append(path) for path in pydis_paths if not path in sys.path]

from framework.disnet_manager import DisNetManager
from pydis import DisNode, DisNet, Cell, CellList
from pydis import CalForce, MobilityLaw, TimeIntegration, Topology
from pydis import Collision, Remesh, VisualizeNetwork, SimulateNetwork

def init_frank_read_src_loop(arm_length=1.0, box_length=8.0, burg_vec=np.array([1.0,0.0,0.0]), pbc=False):
    '''Generate an initial Frank-Read source configuration
    '''
    print("init_frank_read_src_loop: length = %f" % (arm_length))
    cell = Cell(h=box_length*np.eye(3), is_periodic=[pbc,pbc,pbc])
    cell_list = CellList(cell=cell, n_div=[8,8,8])

    rn    = np.array([[0.0, -arm_length/2.0, 0.0,         DisNode.Constraints.PINNED_NODE],
                      [0.0,  0.0,            0.0,         DisNode.Constraints.UNCONSTRAINED],
                      [0.0,  arm_length/2.0, 0.0,         DisNode.Constraints.PINNED_NODE],
                      [0.0,  arm_length/2.0, -arm_length, DisNode.Constraints.PINNED_NODE],
                      [0.0, -arm_length/2.0, -arm_length, DisNode.Constraints.PINNED_NODE]])
    N = rn.shape[0]
    links = np.zeros((N, 8))
    for i in range(N):
        pn = np.cross(burg_vec, rn[(i+1)%N,:3]-rn[i,:3])
        pn = pn / np.linalg.norm(pn)
        links[i,:] = np.concatenate(([i, (i+1)%N], burg_vec, pn))

    return DisNetManager(disnet=DisNet(cell=cell, cell_list=cell_list, rn=rn, links=links))

def main():
    global net, sim
    net = init_frank_read_src_loop(pbc=True)

    bounds = np.array([-0.5*np.diag(net.cell.h), 0.5*np.diag(net.cell.h)])
    vis = VisualizeNetwork(bounds=bounds)

    calforce = CalForce(mu=160e9, nu=0.31, a=0.01, Ec=1.0e6,
                        applied_stress=np.array([0.0, 0.0, 0.0, 0.0, -4.0e6, 0.0]),
                        force_mode='LineTension')

    mobility  = MobilityLaw(mobility_law='SimpleGlide')
    timeint   = TimeIntegration(integrator='EulerForward')
    topology  = Topology(split_mode='MaxDiss')
    collision = Collision(collision_mode='Proximity')
    remesh    = Remesh(remesh_rule='LengthBased', Lmin=0.1, Lmax=0.3)

    sim = SimulateNetwork(calforce=calforce, mobility=mobility,
                          timeint=timeint, topology=topology, collision=collision, remesh=remesh, vis=vis,
                          dt0 = 1.0e-8, max_step=200,
                          print_freq=10, plot_freq=10, plot_pause_seconds=0.1)
    sim.run(net)

    return net.is_sane()


if __name__ == "__main__":
    main()
