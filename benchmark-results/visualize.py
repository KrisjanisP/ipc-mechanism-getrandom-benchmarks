#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict

def parse_time(time_str):
    """Parse time string in format 'm:ss.ss' to seconds"""
    parts = time_str.split(':')
    minutes = int(parts[0])
    seconds = float(parts[1])
    return minutes * 60 + seconds

def main():
    results_file = "/home/kp/progr/trng-dbus-experiments/dbus-benchmark/results.txt"
    
    # Dictionary to store multiple measurements for each configuration
    # Structure: data[bytes_per_call][num_calls] = [list of times]
    raw_data = defaultdict(lambda: defaultdict(list))
    
    with open(results_file, "r") as f:
        for line in f:
            line = line.strip()
            if not line:  # Skip empty lines
                continue
            
            parts = line.split()
            if len(parts) != 3:
                continue
                
            bytes_per_call = int(parts[0])
            num_calls = int(parts[1])
            time_seconds = parse_time(parts[2])
            
            raw_data[bytes_per_call][num_calls].append(time_seconds)
    
    # Process data to calculate statistics
    data = {}
    for bytes_per_call in raw_data:
        data[bytes_per_call] = {
            'num_calls': [],
            'mean_times': [],
            'std_times': [],
            'mean_throughput': [],
            'std_throughput': [],
            'all_times': [],
            'all_throughput': []
        }
        
        for num_calls in sorted(raw_data[bytes_per_call].keys()):
            times = raw_data[bytes_per_call][num_calls]
            throughputs = [num_calls / t if t > 0 else 0 for t in times]
            
            data[bytes_per_call]['num_calls'].append(num_calls)
            data[bytes_per_call]['mean_times'].append(np.mean(times))
            data[bytes_per_call]['std_times'].append(np.std(times))
            data[bytes_per_call]['mean_throughput'].append(np.mean(throughputs))
            data[bytes_per_call]['std_throughput'].append(np.std(throughputs))
            data[bytes_per_call]['all_times'].extend(times)
            data[bytes_per_call]['all_throughput'].extend(throughputs)
    
    # Create subplots - 2x2 layout for comprehensive visualization
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
    
    colors = ['blue', 'orange', 'green']
    markers = ['o', 's', '^']
    
    # Plot 1: Execution time vs number of calls with error bars
    for i, (bytes_per_call, values) in enumerate(sorted(data.items())):
        ax1.errorbar(values['num_calls'], values['mean_times'], 
                    yerr=values['std_times'],
                    label=f"{bytes_per_call} bytes/call", 
                    marker=markers[i % len(markers)],
                    color=colors[i % len(colors)],
                    capsize=5, capthick=2)
    
    ax1.set_xlabel("Number of D-Bus calls")
    ax1.set_ylabel("Execution time (seconds)")
    ax1.set_title("D-Bus Benchmark: Mean Execution Time ± Std Dev")
    ax1.grid(True, alpha=0.3)
    ax1.legend()
    
    # Plot 2: Throughput with error bars
    for i, (bytes_per_call, values) in enumerate(sorted(data.items())):
        ax2.errorbar(values['num_calls'], values['mean_throughput'], 
                    yerr=values['std_throughput'],
                    label=f"{bytes_per_call} bytes/call", 
                    marker=markers[i % len(markers)],
                    color=colors[i % len(colors)],
                    capsize=5, capthick=2)
    
    ax2.set_xlabel("Number of D-Bus calls")
    ax2.set_ylabel("Throughput (calls/second)")
    ax2.set_title("D-Bus Benchmark: Mean Throughput ± Std Dev")
    ax2.grid(True, alpha=0.3)
    ax2.legend()
    
    # Add reference lines for common throughput levels
    ax2.axhline(1000, color='gray', linestyle='--', alpha=0.5)
    ax2.text(ax2.get_xlim()[1]*0.7, 1000*1.05, '1K calls/sec', 
             horizontalalignment='center', color='gray')
    
    ax2.axhline(10000, color='gray', linestyle='--', alpha=0.5)
    ax2.text(ax2.get_xlim()[1]*0.7, 10000*1.05, '10K calls/sec', 
             horizontalalignment='center', color='gray')
    
    # Plot 3: Coefficient of Variation (CV) for execution times
    for i, (bytes_per_call, values) in enumerate(sorted(data.items())):
        # Calculate CV = std/mean * 100
        cv_times = [(std/mean)*100 if mean > 0 else 0 
                   for mean, std in zip(values['mean_times'], values['std_times'])]
        ax3.plot(values['num_calls'], cv_times,
                label=f"{bytes_per_call} bytes/call", 
                marker=markers[i % len(markers)],
                color=colors[i % len(colors)])
    
    ax3.set_xlabel("Number of D-Bus calls")
    ax3.set_ylabel("Coefficient of Variation (%)")
    ax3.set_title("D-Bus Benchmark: Execution Time Variability")
    ax3.grid(True, alpha=0.3)
    ax3.legend()
    
    # Plot 4: Box plot showing distribution for largest test case (50000 calls)
    box_data = []
    box_labels = []
    for bytes_per_call in sorted(data.keys()):
        # Get all times for 50000 calls
        times_50k = []
        for num_calls, times in raw_data[bytes_per_call].items():
            if num_calls == 50000:
                times_50k = times
                break
        if times_50k:
            box_data.append(times_50k)
            box_labels.append(f"{bytes_per_call}B")
    
    if box_data:
        bp = ax4.boxplot(box_data, labels=box_labels, patch_artist=True)
        for patch, color in zip(bp['boxes'], colors[:len(box_data)]):
            patch.set_facecolor(color)
            patch.set_alpha(0.7)
    
    ax4.set_xlabel("Payload size (bytes/call)")
    ax4.set_ylabel("Execution time (seconds)")
    ax4.set_title("Distribution of Execution Times (50K calls)")
    ax4.grid(True, alpha=0.3)
    
    plt.tight_layout()
    plt.savefig("dbus_benchmark_results.png", dpi=300, bbox_inches='tight')
    
    # Print some statistics
    print("\n=== D-Bus Benchmark Statistics ===")
    for bytes_per_call in sorted(data.keys()):
        print(f"\n{bytes_per_call} bytes/call:")
        for i, num_calls in enumerate(data[bytes_per_call]['num_calls']):
            mean_time = data[bytes_per_call]['mean_times'][i]
            std_time = data[bytes_per_call]['std_times'][i]
            cv = (std_time/mean_time)*100 if mean_time > 0 else 0
            print(f"  {num_calls:5d} calls: {mean_time:.3f}±{std_time:.3f}s (CV: {cv:.1f}%)")
    
    plt.show()

if __name__ == "__main__":
    main()
