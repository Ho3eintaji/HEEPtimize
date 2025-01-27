import matplotlib.pyplot as plt

# Data
scenarios = ["CPU", "CGRA", "Carus", "CPU+CGRA+Carus"]
power_consumption = [4.3006539570736235, 4.279259572758497, 4.889146823751835, 6.5713599380324474]

# Plot
plt.figure(figsize=(8, 5))
bars = plt.bar(scenarios, power_consumption, color=['blue', 'green', 'orange', 'red'])
plt.title("Average Power Consumption at 0.8V", fontsize=16)
plt.xlabel("Scenario", fontsize=14)
plt.ylabel("Power Consumption (mW)", fontsize=14)
plt.ylim(0, max(power_consumption) + 1)

# Add values on top of bars
for bar in bars:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2, height, f"{height:.2f} mW", ha='center', va='bottom')

plt.grid(axis='y', linestyle='--', alpha=0.7)
plt.savefig("power_consumption.png")

# Calculate energy consumption (Energy = Power × Time)
energy_consumption = [power * time for power, time in zip(power_consumption, execution_times)]

# Plot
plt.figure(figsize=(8, 5))
bars = plt.bar(scenarios, energy_consumption, color=['blue', 'green', 'orange', 'red'])
plt.title("Energy Consumption Comparison", fontsize=16)
plt.xlabel("Scenario", fontsize=14)
plt.ylabel("Energy Consumption (mW × time units)", fontsize=14)
plt.ylim(0, max(energy_consumption) + 10)

# Add values on top of bars
for bar in bars:
    height = bar.get_height()
    plt.text(bar.get_x() + bar.get_width() / 2, height, f"{height:.2f}", ha='center', va='bottom')

plt.grid(axis='y', linestyle='--', alpha=0.7)
# plt.show()
plt.savefig("energy_consumption_.png")