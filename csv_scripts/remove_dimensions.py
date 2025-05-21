import pandas as pd

df = pd.read_csv('input/vehicle_objects.csv')
df_cleaned = df.drop(columns=['dimensions_uncertainty', 'dimensions'])

# save to input directory because the output file is used in the next step
df_cleaned.to_csv('input/vehicle_objects_nodimensions.csv', index=False)
