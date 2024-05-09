import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("./sample_data/sample_latency_analysis.csv")

plt.figure(figsize=(10, 6))
plt.plot(df["LATENCY"], df["SPOT_BALANCE"], label="Spot Balance")
plt.plot(df["LATENCY"], df["FUTURES_BALANCE"], label="Futures Balance")
plt.xscale("log") 
plt.xlabel("Log Latency (ns)")
plt.ylabel("Account Balance ($)")
plt.title("Account Balance vs. Timestamp")
plt.legend()
plt.grid(True)
plt.show()