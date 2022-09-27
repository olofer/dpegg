#
# EXAMPLES:
#   python dpegg-demo.py --help
#   python dpegg-demo.py --eggs 10 --floors 127 --tiebreak
#   python dpegg-demo.py --dpegg-output dpegg-out-127 --floors 127 --eggs 8 --tiebreak
#   python dpegg-demo.py --dpegg-output dpegg-out-127 --figext png
#

import os
import argparse
import numpy as np
import matplotlib.pyplot as plt

def parse_dpegg_tables(dpeggfile):

  def locate_line(egg, begstr):
    for l in range(len(egg)):
      if egg[l].find(begstr) == 0:
        return l
    return -1

  def parse_table_row(s, begstr):
    if s.find(begstr) != 0:
      return None, []
    isep = s.find(':')
    rownum = int(s[len(begstr):isep])
    rowelems = s[(isep + 1):].split()
    return rownum, [float(e) for e in rowelems]

  def parse_table(egg, start, begstr):
    r = start
    R = []
    while True:
      a, b = parse_table_row(egg[r], begstr)
      if a is None:
        break
      R.append((a, b))
      r += 1
    return R

  with open(dpeggfile, 'r') as thefile:
    egg = thefile.readlines()

    l1 = locate_line(egg, '--- min max drops')
    l2 = locate_line(egg, '--- average drops')
    l3 = locate_line(egg, '--- drop histogram')

    T1 = parse_table(egg, l1 + 1, 'floors')
    T2 = parse_table(egg, l2 + 1, 'floors')
    T3 = parse_table(egg, l3 + 1, 'floor')

    return T1, T2, T3, egg

def convert_to_numpy(T):
  cols = len(T[0][1])
  rows = len(T)
  R = np.tile(np.nan, (rows, ))
  A = np.tile(np.nan, (rows, cols))
  for r in range(rows):
    assert cols == len(T[r][1])
    R[r] = T[r][0]
    A[r, :] = np.asarray(T[r][1])
  return R, A

if __name__ == '__main__':

  parser = argparse.ArgumentParser()

  parser.add_argument('--dpegg-output', type = str, default = 'dpegg-dump', help = 'name of output dump from dpegg (can be pre-existing)')
  parser.add_argument('--eggs', type = int, default = 0, help = 'maximum number of eggs')
  parser.add_argument('--floors', type = int, default = 0, help = 'maximum number of floors')
  parser.add_argument('--tiebreak', action = 'store_true', help = 'sub-optimize the policy to minimize avg. drops')
  parser.add_argument('--horizontals', action = 'store_true', help = 'plot optics')
  parser.add_argument('--maxfticks', type = int, default = 12, help = 'plot optics')
  parser.add_argument('--figext', type = str, default = 'pdf', help = 'figure file extension (e.g. pdf or png)')
  parser.add_argument('--dpi', type = float, default = 250.0, help = 'print to file PNG option')

  args = parser.parse_args()

  assert len(args.dpegg_output) > 0

  if args.eggs >= 1 and args.floors >= 1:

    cmdstring = './dpegg {} {}'.format(args.floors, args.eggs)
    if args.tiebreak:
      cmdstring += ' --tiebreak'
    cmdstring += ' > {}'.format(args.dpegg_output)
    print(cmdstring)
    retcode = os.system(cmdstring)
    assert retcode == 0

  if len(args.dpegg_output) > 0:

    T1, T2, T3, egg = parse_dpegg_tables(args.dpegg_output)

    print('read {} lines from: {}'.format(len(egg), args.dpegg_output))

    used_tiebreak = (egg[0].find('--tiebreak') != -1)

    assert len(T1) == len(T2)
    assert len(T1) == len(T3)

    F1, D1 = convert_to_numpy(T1)
    F2, D2 = convert_to_numpy(T2)
    F3, D3 = convert_to_numpy(T3)

    assert np.all(F1 == F2)
    assert np.all(F1 == F3)
    assert np.all(D1 >= D2)  # min max #drops >= mean drops

    F = F1
    E = np.arange(1, D1.shape[1] + 1)

    Dhat = D3 / len(F)

    dpi_option = args.dpi if args.figext == 'png' else 'figure'

    plt.imshow(Dhat.transpose(), interpolation = 'nearest', origin = 'lower', aspect = 'auto')
    plt.colorbar()
    if args.horizontals:
      for e in E:
        plt.axhline(y = e - 0.5, c = 'w', alpha = 0.50)
    plt.gca().set_yticks(np.arange(Dhat.shape[1]), labels = E)
    nxticks = args.maxfticks if Dhat.shape[0] > args.maxfticks else Dhat.shape[0]
    labs = [int(np.round(e)) for e in np.linspace(0, Dhat.shape[0] - 1, nxticks)]
    floor_labs = [str(int(F[e])) for e in labs]
    plt.gca().set_xticks(labs, labels = floor_labs, minor = False)
    plt.gca().set_xticks(np.arange(Dhat.shape[0]), labels = None, minor = True)
    plt.text(F[-1] - 1, E[-1] - 1, 'tiebreak: {}'.format(used_tiebreak), 
             horizontalalignment = 'right', c = 'w', fontsize = 6)
    plt.xlabel('Floor')
    plt.ylabel('Egg budget')
    plt.title('Access frequencies, floors 1..{}'.format(len(F)))
    plt.tight_layout()
    plt.savefig('{}-access.{}'.format(args.dpegg_output, args.figext), dpi = dpi_option)
    plt.close()

    plt.plot(E, D1[-1, :], c = 'r', label = 'min max', marker = 's', alpha = 0.75, markersize = 10)
    plt.plot(E, D2[-1, :], c = 'b', label = 'average', marker = 'o', alpha = 0.75, markersize = 10)
    plt.ylim([D2[-1, -1] - 1, D1[-1, 1] + 1])
    plt.text(0.5, D1[-1, 1], '{:.0f} (n=1)'.format(D1[-1, 0]), c = 'r', fontweight = 'bold')
    plt.text(0.5, D1[-1, 1] - 0.50, '{:.2f} (n=1)'.format(D2[-1, 0]), c = 'b', fontweight = 'bold')
    plt.axhline(y = D1[-1, -1], color = 'r', linestyle ='--', alpha = 0.50)
    plt.text(1.0, D1[-1, -1], '{:.0f}'.format(D1[-1, -1]), c = 'r', fontweight = 'bold', verticalalignment = 'bottom')
    plt.axhline(y = D2[-1, -1], color = 'b', linestyle ='--', alpha = 0.50)
    plt.text(1.0, D2[-1, -1], '{:.3f}'.format(D2[-1, -1]), c = 'b', fontweight = 'bold', verticalalignment = 'top')
    plt.text(E[-1], D1[-1, 1] - 1, 'tiebreak: {}'.format(used_tiebreak), 
             horizontalalignment = 'right', c = 'k', fontsize = 6, fontweight = 'bold')
    plt.legend()
    plt.grid(True)
    plt.title('{} floors'.format(len(F)))
    plt.gca().set_xticks(E, labels = E)
    plt.xlabel('Egg budget')
    plt.ylabel('Number of drops')
    plt.tight_layout()
    plt.savefig('{}-drops.{}'.format(args.dpegg_output, args.figext), dpi = dpi_option)
    plt.close()

  print('done.')
