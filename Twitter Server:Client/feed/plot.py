import matplotlib.pyplot as plt


x = [2, 4, 6, 8, 12]
xsmall = [1.11,	1.085,	1.073,	1.035,	1.09]
small = [1.073,	0.99,	0.99,	1.016,	1]
medium = [1.03,	1.148,	1.186,	1.21,	1.226]
large = [1.259,	1.41,	1.614,	1.69,	1.91]
xlarge = [1.34,	1.88,	2.243,	2.45,	2.7]


plt.figure()
plt.plot(x, xsmall, label="xsmall")
plt.plot(x, small, label="small")
plt.plot(x, medium, label="medium")
plt.plot(x, large, label="large")
plt.plot(x, xlarge, label="xlarge")

plt.xlabel('num of threads')
plt.ylabel('speedup ')
# Add legend
plt.legend()
plt.show()