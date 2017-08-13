#!/usr/bin/python
import numpy as np
import subprocess
import matplotlib.pyplot as plt
import scipy.signal
import os

tenbylog10 = 10.0/np.log(10.0)
PtodB = lambda p: tenbylog10*np.log(p)

fS = 48000     # sample rate
fNy = fS / 2.0 # Nyquist Frequency
T  = 1.0       # 1 second
N = int(T*fS)  # number of samples

t = np.linspace(0.0, T, N, False)

# original data
x = np.random.normal(scale=.01, size=N)
fx, Px = scipy.signal.welch(x, fS)

# filter data
b, a = scipy.signal.iirfilter(4, (2000/fNy, 5000/fNy), 3.0, 40.0 )
y = scipy.signal.lfilter(b, a, x)
fy, Py = scipy.signal.welch(y, fS)

# write filter taps for external program, then filter
# data is 32bit signed integers with +/-1 == +/- 2^24
# also coefficients are scaled by 2^24
with open('taps.txt','wt') as f :
	print(len(b), file=f)
	for i in range(1, len(a)) :
		print(int(a[i]*(1<<24)), file=f)
	for i in range(0, len(a)) :
		print(int(b[i]*(1<<24)), file=f)
x_raw = (x*(1<<24)).astype(np.int32)
x_raw.tofile('x_raw.bin')
try :
	os.unlink('y_raw.bin')
except FileNotFoundError as e :
	pass
subprocess.run(["./fir_filter_test", "taps.txt", "x_raw.bin", "y_raw.bin"])
y_raw = np.fromfile('y_raw.bin', np.int32)
y_ext = y_raw.astype(np.float)*(1.0/(1<<24))
print('Read %d samples from ext. file'%(y_ext.shape[0]))

# calculate difference
dy = y_ext - y

fye, Pye = scipy.signal.welch(y_ext, fS)
fd, Pd = scipy.signal.welch(dy, fS)

fig = plt.figure()
ax = fig.add_subplot(1,1,1)
# ax.semilogx(fx,PtodB(Px), label='Raw')
ax.loglog(fy, Py, label='Filt')
ax.loglog(fye,Pye, label='Ext')
ax.loglog(fd, Pd, label='Diff')
ax.grid(True, which='major', linestyle='-')
ax.grid(True, which='minor', linestyle='--')
ax.legend()
fig.show()

fig = plt.figure()
ax = fig.add_subplot(1,1,1)
# ax.plot(t[1000:1200], x[1000:1200])
ax.plot(t[1000:1200], y[1000:1200])
ax.plot(t[1000:1200], y_ext[1000:1200])
ax.grid()
fig.show()
