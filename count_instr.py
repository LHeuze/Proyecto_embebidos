import re
from collections import Counter
import matplotlib.pyplot as plt

# Path to the assembly file
asm_file_path = 'tasks_asm/conv_task.txt'

# Read the assembly file
with open(asm_file_path, 'r') as file:
    asm_content = file.readlines()

# Initialize a counter for instructions
instruction_counter = Counter()

# Process each line in the assembly file
for line in asm_content:
    parts = line.split()
    # Ensure the line has at least three parts to avoid index errors
    if len(parts) > 2:
        instruction = parts[2]
        instruction_counter[instruction] += 1

# Select the top N instructions to display
top_n = 25
top_instructions = instruction_counter.most_common(top_n)
instructions, counts = zip(*top_instructions)

# Plotting the pie chart
plt.figure(figsize=(10, 7))
plt.pie(counts, labels=instructions, autopct='%1.1f%%', startangle=140)
plt.title(f"Top {top_n} instrucciones mas repetidas")
plt.axis('equal')  # Equal aspect ratio ensures that pie is drawn as a circle.
plt.savefig('conv_piechart.png')


