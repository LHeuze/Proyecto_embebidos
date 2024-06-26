import matplotlib.pyplot as plt

# Data from the output
tasks = ['Total', 'Fully Connected', 'Depthwise Conv', 'Conv', 'Pooling', 'Add', 'Mul']
times = [213, 0, 0, 205, 7, 0, 0]  # in milliseconds
energies = [168.972397, 0.068904, 0.0, 162.771042, 5.921784, 0.0, 0.0]  # in millijoules

# Plotting the execution times
plt.figure(figsize=(12, 6))

# Plot execution times
plt.subplot(1, 2, 1)
plt.barh(tasks, times, color='skyblue')
plt.xlabel('Execution Time (ms)')
plt.title('tiempo de ejecucion de cada subtask')

# Plot energy consumption
plt.subplot(1, 2, 2)
plt.barh(tasks, energies, color='salmon')
plt.xlabel('Energy Consumption (mJ)')
plt.title('Consumo de energia de cada subtask')

# Adjust layout
plt.tight_layout()
plt.savefig('bar_times.png')