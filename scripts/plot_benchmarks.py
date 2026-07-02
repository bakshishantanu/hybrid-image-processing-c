import pandas as pd
import matplotlib.pyplot as plt
import os
import numpy as np

RESULTS_DIR = 'results'

def get_model_label(row):
    model = row['model']
    if model == 'openmp':
        return f"OMP {row['threads']}t"
    elif model == 'hybrid':
        return f"HB {row['processes']}p"
    else:
        return model.upper()

def plot_strong_scaling():
    csv_file = os.path.join(RESULTS_DIR, 'strong_scaling.csv')
    if not os.path.exists(csv_file):
        return
        
    df = pd.read_csv(csv_file)
    models = [get_model_label(row) for _, row in df.iterrows()]
    times = df['total_ms'].tolist()
        
    plt.figure(figsize=(10, 6))
    plt.bar(models, times, color=['blue' if 'OMP' in m else 'green' if 'HB' in m else 'red' if 'CUDA' in m else 'gray' for m in models])
    plt.ylabel('Total Execution Time (ms)')
    plt.title(f'Strong Scaling on {df["image"].iloc[0]}')
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, 'strong_scaling_plot.png'))
    print("Saved strong_scaling_plot.png")

def plot_size_scaling():
    csv_file = os.path.join(RESULTS_DIR, 'size_scaling.csv')
    if not os.path.exists(csv_file):
        return
        
    df = pd.read_csv(csv_file)
    plt.figure(figsize=(10, 6))
    
    for model in df['model'].unique():
        df_model = df[df['model'] == model].sort_values(by='pixels')
        plt.plot(df_model['pixels'], df_model['total_ms'], marker='o', label=model.upper())
        
    plt.xscale('log')
    plt.yscale('log')
    plt.xlabel('Image Size (pixels)')
    plt.ylabel('Total Execution Time (ms)')
    plt.title('Problem Size Scaling')
    plt.legend()
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, 'size_scaling_plot.png'))
    print("Saved size_scaling_plot.png")

def plot_speedup():
    csv_file = os.path.join(RESULTS_DIR, 'strong_scaling.csv')
    if not os.path.exists(csv_file):
        return
        
    df = pd.read_csv(csv_file)
    seq_time = df[df['model'] == 'sequential']['total_ms'].iloc[0]
    
    # Isolate just OpenMP for the speedup curve
    df_omp = df[df['model'] == 'openmp'].sort_values(by='threads')
    threads = df_omp['threads'].tolist()
    speedups = [seq_time / t for t in df_omp['total_ms']]
    
    plt.figure(figsize=(8, 6))
    plt.plot(threads, speedups, marker='o', linestyle='-', color='blue', label='OpenMP Speedup')
    
    # Add ideal linear speedup line for reference
    plt.plot(threads, threads, linestyle='--', color='gray', label='Ideal Linear Speedup')
    
    plt.xlabel('Number of Threads')
    plt.ylabel('Speedup (T_seq / T_p)')
    plt.title('CPU Multi-threading Speedup (OpenMP)')
    plt.legend()
    plt.grid(True, alpha=0.5)
    plt.xticks(threads)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, 'speedup_plot.png'))
    print("Saved speedup_plot.png")

def plot_execution_breakdown():
    csv_file = os.path.join(RESULTS_DIR, 'strong_scaling.csv')
    if not os.path.exists(csv_file):
        return
        
    df = pd.read_csv(csv_file)
    
    # We'll plot sequential, best OMP, best Hybrid, and CUDA
    models_to_plot = []
    
    # Find sequential
    seq_row = df[df['model'] == 'sequential'].iloc[0]
    models_to_plot.append(seq_row)
    
    # Find best OMP
    omp_rows = df[df['model'] == 'openmp']
    if not omp_rows.empty:
        best_omp = omp_rows.loc[omp_rows['total_ms'].idxmin()]
        models_to_plot.append(best_omp)
        
    # Find best Hybrid
    hb_rows = df[df['model'] == 'hybrid']
    if not hb_rows.empty:
        best_hb = hb_rows.loc[hb_rows['total_ms'].idxmin()]
        models_to_plot.append(best_hb)
        
    # Find CUDA
    cu_rows = df[df['model'] == 'cuda']
    if not cu_rows.empty:
        models_to_plot.append(cu_rows.iloc[0])
        
    df_plot = pd.DataFrame(models_to_plot)
    labels = [get_model_label(row) for _, row in df_plot.iterrows()]
    
    gray = df_plot['gray_ms'].values
    blur = df_plot['blur_ms'].values
    sobel = df_plot['sobel_ms'].values
    unsharp = df_plot['unsharp_ms'].values
    
    x = np.arange(len(labels))
    width = 0.5
    
    plt.figure(figsize=(10, 6))
    p1 = plt.bar(x, gray, width, label='Grayscale', color='#4c72b0')
    p2 = plt.bar(x, blur, width, bottom=gray, label='Gaussian Blur', color='#dd8452')
    p3 = plt.bar(x, sobel, width, bottom=gray+blur, label='Sobel Filter', color='#55a868')
    p4 = plt.bar(x, unsharp, width, bottom=gray+blur+sobel, label='Unsharp Mask', color='#c44e52')
    
    plt.ylabel('Execution Time (ms)')
    plt.title('Filter Execution Time Breakdown')
    plt.xticks(x, labels)
    plt.legend()
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, 'execution_breakdown_plot.png'))
    print("Saved execution_breakdown_plot.png")

if __name__ == "__main__":
    if not os.path.exists(RESULTS_DIR):
        print(f"Directory {RESULTS_DIR} not found.")
    else:
        plot_strong_scaling()
        plot_size_scaling()
        plot_speedup()
        plot_execution_breakdown()
