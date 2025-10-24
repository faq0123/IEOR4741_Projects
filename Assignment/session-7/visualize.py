#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import glob
import os

sns.set_style("whitegrid")

def parse_performance_file(filename):
    """Parse performance CSV and extract metrics"""
    df = pd.read_csv(filename)
    metrics = {}
    for _, row in df.iterrows():
        metrics[row['Metric']] = float(row['Value'])
    return metrics

def parse_filename(filename):
    """Extract rate and threads from filename"""
    basename = os.path.basename(filename)
    parts = basename.replace('.csv', '').split('_')
    if len(parts) >= 3:
        rate = int(parts[1])
        threads = int(parts[2])
        return rate, threads
    return None, None

def plot_performance_comparison():
    """Create comparison charts for different configurations"""
    
    # Find all performance files
    perf_files = glob.glob('benchmark_results/performance_*.csv')
    
    if not perf_files:
        print("No benchmark results found!")
        print("Run ./benchmark.sh first")
        return
    
    # Collect data
    data = []
    for file in perf_files:
        rate, threads = parse_filename(file)
        if rate is None:
            continue
        metrics = parse_performance_file(file)
        data.append({
            'Rate': rate,
            'Threads': threads,
            'Throughput': metrics.get('Throughput(orders/s)', 0),
            'AvgLatency': metrics.get('AvgLatency(us)', 0),
            'TotalOrders': metrics.get('TotalOrders', 0)
        })
    
    df = pd.DataFrame(data)
    df = df.sort_values('Rate')
    
    # Create figure with subplots
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('HFT System Performance Analysis', fontsize=16, fontweight='bold')
    
    # 1. Throughput vs Market Rate
    ax1 = axes[0, 0]
    for threads in df['Threads'].unique():
        subset = df[df['Threads'] == threads]
        ax1.plot(subset['Rate'], subset['Throughput'], 
                marker='o', label=f'{threads} threads', linewidth=2)
    ax1.set_xlabel('Market Data Rate (updates/s)', fontsize=11)
    ax1.set_ylabel('Throughput (orders/s)', fontsize=11)
    ax1.set_title('Throughput vs Market Data Rate', fontweight='bold')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # 2. Latency vs Market Rate
    ax2 = axes[0, 1]
    for threads in df['Threads'].unique():
        subset = df[df['Threads'] == threads]
        ax2.plot(subset['Rate'], subset['AvgLatency'], 
                marker='s', label=f'{threads} threads', linewidth=2)
    ax2.set_xlabel('Market Data Rate (updates/s)', fontsize=11)
    ax2.set_ylabel('Average Latency (μs)', fontsize=11)
    ax2.set_title('Latency vs Market Data Rate', fontweight='bold')
    ax2.legend()
    ax2.grid(True, alpha=0.3)
    
    # 3. Threads Impact on Throughput
    ax3 = axes[1, 0]
    # Group by rate for comparison
    pivot_throughput = df.pivot(index='Threads', columns='Rate', values='Throughput')
    pivot_throughput.plot(kind='bar', ax=ax3, width=0.8)
    ax3.set_xlabel('Number of Strategy Threads', fontsize=11)
    ax3.set_ylabel('Throughput (orders/s)', fontsize=11)
    ax3.set_title('Thread Scaling Analysis', fontweight='bold')
    ax3.legend(title='Market Rate', bbox_to_anchor=(1.05, 1))
    ax3.grid(True, alpha=0.3, axis='y')
    
    # 4. Total Orders Processed
    ax4 = axes[1, 1]
    bar_width = 0.35
    x = range(len(df))
    colors = plt.cm.viridis(df['Threads'] / df['Threads'].max())
    bars = ax4.bar(x, df['TotalOrders'], color=colors, width=bar_width)
    ax4.set_xlabel('Configuration', fontsize=11)
    ax4.set_ylabel('Total Orders Processed', fontsize=11)
    ax4.set_title('Order Volume by Configuration', fontweight='bold')
    ax4.set_xticks(x)
    ax4.set_xticklabels([f"{row['Rate']//1000}K/{row['Threads']}T" 
                         for _, row in df.iterrows()], rotation=45)
    ax4.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig('benchmark_results/performance_analysis.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: benchmark_results/performance_analysis.png")
    
    # Create summary table
    summary_df = df[['Rate', 'Threads', 'Throughput', 'AvgLatency', 'TotalOrders']].copy()
    summary_df.columns = ['Market Rate', 'Threads', 'Throughput (ord/s)', 
                          'Avg Latency (μs)', 'Total Orders']
    summary_df['Market Rate'] = summary_df['Market Rate'].apply(lambda x: f"{x:,}")
    summary_df['Throughput (ord/s)'] = summary_df['Throughput (ord/s)'].apply(lambda x: f"{x:.1f}")
    summary_df['Avg Latency (μs)'] = summary_df['Avg Latency (μs)'].apply(lambda x: f"{x:.2f}")
    
    print("\n" + "="*80)
    print("PERFORMANCE SUMMARY")
    print("="*80)
    print(summary_df.to_string(index=False))
    print("="*80)

def plot_latency_distribution():
    """Plot latency distributions from order logs"""
    
    order_files = glob.glob('benchmark_results/orders_*.csv')
    
    if not order_files:
        print("No order logs found!")
        return
    
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Latency Distribution Analysis', fontsize=16, fontweight='bold')
    
    for idx, file in enumerate(order_files[:4]):  # Plot first 4 configs
        if idx >= 4:
            break
        
        df = pd.read_csv(file)
        rate, threads = parse_filename(file)
        
        ax = axes[idx // 2, idx % 2]
        
        # Histogram
        ax.hist(df['Latency_us'], bins=50, alpha=0.7, color='skyblue', edgecolor='black')
        ax.set_xlabel('Latency (μs)', fontsize=11)
        ax.set_ylabel('Frequency', fontsize=11)
        ax.set_title(f'Rate: {rate:,}/s | Threads: {threads}', fontweight='bold')
        ax.grid(True, alpha=0.3, axis='y')
        
        # Stats
        mean_lat = df['Latency_us'].mean()
        median_lat = df['Latency_us'].median()
        ax.axvline(mean_lat, color='red', linestyle='--', linewidth=2, label=f'Mean: {mean_lat:.1f}')
        ax.axvline(median_lat, color='green', linestyle='--', linewidth=2, label=f'Median: {median_lat:.1f}')
        ax.legend()
    
    plt.tight_layout()
    plt.savefig('benchmark_results/latency_distributions.png', dpi=300, bbox_inches='tight')
    print("✓ Saved: benchmark_results/latency_distributions.png")

if __name__ == '__main__':
    print("\n" + "="*80)
    print("HFT PERFORMANCE VISUALIZATION")
    print("="*80 + "\n")
    
    plot_performance_comparison()
    print()
    plot_latency_distribution()
    
    print("\n" + "="*80)
    print("Visualization complete!")
    print("="*80 + "\n")