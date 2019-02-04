
import matplotlib.pylab as plt
from matplotlib.pyplot import figure, show
import matplotlib.pyplot as pyplot
import matplotlib.artist as artist
import scipy.sparse as sps
import scipy.io
from numpy import linalg as LA
from numpy import arange, concatenate, trace
import math

fig = figure(num=None, figsize=(20, 25), dpi=100, facecolor='w', edgecolor='k')
#fig, ax = plt.subplots(4, 2, figsize=(20, 9), dpi=100)

A1 = scipy.io.mmread("/Users/jnbrunet/sources/caribou/matrix.mtx")
M1 = sps.csr_matrix(A1)

ax = fig.add_subplot(1, 1, 1)
ax.set_title('STIFFNESS')
ax.spy(M1,precision=0.0000001, markersize=0.1)
ax.grid(axis='x')
pyplot.setp(ax, xticks=arange(0, M1.shape[1]+1, (M1.shape[1]+1)/10.0))

show()