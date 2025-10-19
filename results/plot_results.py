import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

def generate_plots():
    results_dir = os.path.dirname(os.path.abspath(__file__))
    csv_path = os.path.join(results_dir, 'benchmark_results.csv')
    
    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found.")
        return

    df = pd.read_csv(csv_path)
    df['Cipher'] = df['Cipher'].astype('category')
    df['Operation'] = df['Operation'].astype('category')

    # --- Plot 1: Throughput for Largest File with Standard Deviation ---
    plt.style.use('seaborn-v0_8-whitegrid')
    
    df_large_file = df[df['Filename'].str.contains('2_5MB')].copy()
    df_large_file['Throughput_StdDev'] = (df_large_file['Throughput(MB/s)'] / df_large_file['MeanTime(ms)']) * df_large_file['StdDev(ms)']

    fig, ax = plt.subplots(figsize=(12, 7))
    sns.barplot(data=df_large_file, x='Cipher', y='Throughput(MB/s)', hue='Operation', ax=ax, palette='viridis')

    # Robustly add error bars
    for bar in ax.patches:
        # Find the corresponding data point for the bar
        x_val = ax.get_xticklabels()[int(round(bar.get_x() + bar.get_width() / 2))].get_text()
        
        # Determine hue by bar color (a bit of a hack, but effective)
        bar_color = bar.get_facecolor()
        hue_val = 'encrypt' if bar_color == sns.color_palette('viridis', 2)[0] else 'decrypt'
        
        row = df_large_file[(df_large_file['Cipher'] == x_val) & (df_large_file['Operation'] == hue_val)]
        if not row.empty:
            error = row['Throughput_StdDev'].iloc[0]
            ax.errorbar(bar.get_x() + bar.get_width() / 2, bar.get_height(), yerr=error, fmt='none', c='black', capsize=4)

    ax.set_title('Throughput for 2.5MB File (Larger is Better)', fontsize=16, weight='bold')
    ax.set_ylabel('Throughput (MB/s)', fontsize=12)
    ax.set_xlabel('Cipher', fontsize=12)
    ax.legend(title='Operation')

    throughput_plot_path = os.path.join(results_dir, 'throughput_comparison_with_stddev.png')
    plt.savefig(throughput_plot_path, dpi=300, bbox_inches='tight')
    plt.close(fig)
    print(f"Saved throughput plot to: {throughput_plot_path}")


    # --- Plot 2: Performance (Time) by File Size with Standard Deviation ---
    g = sns.catplot(
        data=df,
        kind='bar',
        x='Cipher',
        y='MeanTime(ms)',
        hue='Operation',
        col='Filename',
        palette='plasma',
        height=5,
        aspect=0.9,
        sharey=False,
        errorbar=None
    )

    g.figure.suptitle('Mean Execution Time by File Size (Smaller is Better)', fontsize=16, weight='bold', y=1.03)
    g.set_axis_labels("Cipher", "Mean Time (ms)")
    g.set_titles("File: {col_name}")
    g.despine(left=True)

    # Robustly add error bars to each facet
    for i, ax in enumerate(g.axes.flat):
        ax.set_yscale('log')
        filename = g.col_names[i]
        sub_df = df[df['Filename'] == filename]
        
        for bar in ax.patches:
            x_val = ax.get_xticklabels()[int(round(bar.get_x() + bar.get_width() / 2))].get_text()
            
            bar_color = bar.get_facecolor()
            hue_val = 'encrypt' if bar_color == sns.color_palette('plasma', 2)[0] else 'decrypt'
            
            row = sub_df[(sub_df['Cipher'] == x_val) & (sub_df['Operation'] == hue_val)]
            if not row.empty:
                error = row['StdDev(ms)'].iloc[0]
                ax.errorbar(bar.get_x() + bar.get_width() / 2, bar.get_height(), yerr=error, fmt='none', c='black', capsize=3)

    performance_plot_path = os.path.join(results_dir, 'performance_comparison_with_stddev.png')
    plt.savefig(performance_plot_path, dpi=300, bbox_inches='tight')
    plt.close(g.fig)
    print(f"Saved performance plot to: {performance_plot_path}")

if __name__ == '__main__':
    generate_plots()
