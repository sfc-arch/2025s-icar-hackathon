import pandas as pd

input_path = "input/vehicle_objects_10min_nodimensions.csv"
output_path = "input/10min_1_2.csv"
df = pd.read_csv(input_path)
filtered_df = df[df['sensor_id'].isin(['vista-p90-1', 'vista-p90-2'])]
filtered_df.to_csv(output_path, index=False)

print(f"Filtered data saved to {output_path}")
