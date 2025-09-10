#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
import os

def parse_time(time_str):
    """Parse time string in format 'm:ss.ss' to seconds"""
    parts = time_str.split(':')
    minutes = int(parts[0])
    seconds = float(parts[1])
    return minutes * 60 + seconds

def parse_file(filepath):
    """Parse benchmark file and return structured data"""
    raw_data = defaultdict(lambda: defaultdict(list))
    
    if not os.path.exists(filepath):
        print(f"Warning: File {filepath} not found")
        return {}
    
    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if not line:  # Skip empty lines
                continue
            
            parts = line.split()
            if len(parts) < 4:  # run_id, bytes_per_call, num_calls, time
                continue
                
            run_id = int(parts[0])
            bytes_per_call = int(parts[1])
            num_calls = int(parts[2])
            time_seconds = parse_time(parts[3])

            if (time_seconds < 2**-6):
                continue
            
            raw_data[bytes_per_call][num_calls].append(time_seconds)
    
    # Process data to calculate statistics
    data = {}
    for bytes_per_call in raw_data:
        data[bytes_per_call] = {
            'num_calls': [],
            'mean_times': [],
            'std_times': [],
            'mean_throughput': [],
            'std_throughput': []
        }
        
        for num_calls in sorted(raw_data[bytes_per_call].keys()):
            times = raw_data[bytes_per_call][num_calls]
            throughputs = [num_calls / t if t > 0 else 0 for t in times]
            
            data[bytes_per_call]['num_calls'].append(num_calls)
            data[bytes_per_call]['mean_times'].append(np.mean(times))
            data[bytes_per_call]['std_times'].append(np.std(times))
            data[bytes_per_call]['mean_throughput'].append(np.mean(throughputs))
            data[bytes_per_call]['std_throughput'].append(np.std(throughputs))
    
    return data

def create_comparison_plot(data_dict, title, filename, payload_sizes=[1, 32, 1024], is_large=False):
    """Create a comparison plot for different IPC methods with multiple payload sizes"""
    fig, ax = plt.subplots(1, 1, figsize=(9, 5))
    
    colors = {'dbus': '#1f77b4', 'grpc': '#ff7f0e', 'socket': '#2ca02c'}
    markers = {'dbus': 'o', 'grpc': 's', 'socket': '^'}
    linestyles = ['-', '--']
    
    # Plot each IPC method
    for method, data in data_dict.items():
        if not data:  # Skip if no data
            continue
            
        # For each payload size, plot the execution times
        for i, payload_size in enumerate(payload_sizes):
            if payload_size in data:
                # Format payload size for legend
                if is_large:
                    size_mb = payload_size // (1024 * 1024)
                    size_label = f"{size_mb}MiB"
                else:
                    size_label = f"{payload_size}B"
                
                label = f"{method.upper()} ({size_label})"
                
                ax.errorbar(data[payload_size]['num_calls'], 
                           data[payload_size]['mean_times'],
                           yerr=data[payload_size]['std_times'],
                           label=label,
                           marker=markers[method],
                           color=colors[method],
                           linestyle=linestyles[i % len(linestyles)],
                           capsize=3, capthick=1.5,
                           markersize=6, linewidth=2,
                           alpha=0.8)
    
    # Formatting
    ax.set_xlabel("Number of calls", fontsize=14, fontweight='medium')
    ax.set_ylabel("Wall time (seconds)", fontsize=14, fontweight='medium')
    ax.set_title(title, fontsize=16, fontweight='medium', pad=20)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=10, loc='lower right', ncol=1)
    
    # Increase tick label sizes
    ax.tick_params(axis='both', which='major', labelsize=12)
    
    # Add horizontal reference lines for better overview
    ax.axhline(y=1.0, color='darkgreen', linestyle=':', alpha=0.7, linewidth=1.5, label='1 second')
    ax.axhline(y=60.0, color='darkred', linestyle=':', alpha=0.7, linewidth=1.5, label='1 minute')
    
    # Use log scale for better visualization if needed
    max_time = 0
    for method, data in data_dict.items():
        if data:
            for ps in payload_sizes:
                if ps in data and data[ps]['mean_times']:
                    max_time = max(max_time, max(data[ps]['mean_times']))
    
    if max_time > 10:
        ax.set_yscale('log', base=2)
    
    plt.tight_layout()
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()
    print(f"Saved: {filename}")

def print_statistics(data_dict, title):
    """Print statistics for comparison"""
    print(f"\n{'='*50}")
    print(f"{title}")
    print(f"{'='*50}")
    
    for method, data in data_dict.items():
        if not data:
            continue
            
        print(f"\n{method.upper()} Performance:")
        print("-" * 30)
        
        for bytes_per_call in sorted(data.keys()):
            print(f"\n{bytes_per_call} bytes/call:")
            for i, num_calls in enumerate(data[bytes_per_call]['num_calls']):
                mean_time = data[bytes_per_call]['mean_times'][i]
                std_time = data[bytes_per_call]['std_times'][i]
                mean_throughput = data[bytes_per_call]['mean_throughput'][i]
                cv = (std_time/mean_time)*100 if mean_time > 0 else 0
                print(f"  {num_calls:6d} calls: {mean_time:7.3f}±{std_time:5.3f}s "
                      f"({mean_throughput:8.1f} calls/sec, CV: {cv:4.1f}%)")

def main():
    base_path = "/home/kp/progr/trng-dbus-experiments/benchmark-results"
    
    # File paths
    files = {
        'regular': {
            'dbus': f"{base_path}/dbus.txt",
            'grpc': f"{base_path}/grpc.txt", 
            'socket': f"{base_path}/socket.txt"
        },
        'large': {
            'dbus': f"{base_path}/dbus_large.txt",
            'grpc': f"{base_path}/grpc_large.txt",
            'socket': f"{base_path}/socket_large.txt"
        }
    }
    
    # Parse all data files
    print("Parsing benchmark data files...")
    
    regular_data = {}
    large_data = {}
    
    for method in ['dbus', 'grpc', 'socket']:
        regular_data[method] = parse_file(files['regular'][method])
        large_data[method] = parse_file(files['large'][method])
    
    # Create comparison plots
    print("\nCreating comparison plots...")
    
    # Regular data plot - 2 payload sizes (1, 1024 bytes) on one plot
    payload_sizes = [1, 1024]  # Small and large regular payloads
    create_comparison_plot(
        regular_data,
        "IPC mechanism comparison - small payloads",
        "ipc_comparison_regular.png",
        payload_sizes,
        is_large=False
    )
    
    # Large data plot - 2 payload sizes (10MB, 30MB) on one plot  
    large_payload_sizes = [10485760, 31457280]  # 10MB, 30MB
    create_comparison_plot(
        large_data,
        "IPC mechanism comparison - large payloads",
        "ipc_comparison_large.png",
        large_payload_sizes,
        is_large=True
    )
    
    # Print comprehensive statistics
    print_statistics(regular_data, "REGULAR PAYLOAD BENCHMARKS")
    print_statistics(large_data, "LARGE PAYLOAD BENCHMARKS")
    
    # Summary comparison
    print(f"\n{'='*60}")
    print("PERFORMANCE SUMMARY")
    print(f"{'='*60}")
    
    # Compare throughput at specific test points
    test_points = [
        ('Regular', 1000, 1),
        ('Regular', 10000, 32), 
        ('Regular', 50000, 1024),
        ('Large', 25, 10485760),
        ('Large', 50, 20971520)
    ]
    
    for test_type, num_calls, payload_size in test_points:
        print(f"\n{test_type} test: {num_calls} calls, {payload_size} bytes/call")
        print("-" * 50)
        
        data_set = regular_data if test_type == 'Regular' else large_data
        
        for method in ['socket', 'dbus', 'grpc']:  # Order by typical performance
            if method in data_set and payload_size in data_set[method]:
                method_data = data_set[method][payload_size]
                if num_calls in method_data['num_calls']:
                    idx = method_data['num_calls'].index(num_calls)
                    mean_time = method_data['mean_times'][idx]
                    throughput = method_data['mean_throughput'][idx]
                    print(f"{method.upper():>8}: {mean_time:7.3f}s ({throughput:8.1f} calls/sec)")

if __name__ == "__main__":
    main()
