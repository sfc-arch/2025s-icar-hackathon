import pandas as pd
from geopy.distance import geodesic

vehicle_data = pd.read_csv('vehicle_objects.csv')
vehicle_data['timestamp'] = pd.to_datetime(vehicle_data['timestamp'])

print("Filtering data to only include the first 5 minute")
start_time = vehicle_data['timestamp'].min()
end_time = start_time + pd.Timedelta(minutes=5)
vehicle_data = vehicle_data[(vehicle_data['timestamp'] >= start_time) & (vehicle_data['timestamp'] <= end_time)]

print("Filtering data for vista-p90-2 and vista-p90-3")
group_data = vehicle_data[vehicle_data['sensor_id'].isin(['vista-p90-2', 'vista-p90-3'])].copy()

group_data.sort_values(by='timestamp', inplace=True)

print("Separating data for each sensor")
vista_p90_2 = group_data[group_data['sensor_id'] == 'vista-p90-2']
vista_p90_3 = group_data[group_data['sensor_id'] == 'vista-p90-3']

def is_match(row1, row2, max_distance=10, max_seconds=5, max_heading_diff=30):
    dist = geodesic((row1.lat, row1.lon), (row2.lat, row2.lon)).meters
    time_diff = abs((row1.timestamp - row2.timestamp).total_seconds())
    heading_diff = abs(row1.heading - row2.heading)
    return dist <= max_distance and time_diff <= max_seconds and heading_diff <= max_heading_diff

print("Matching")
matches = {}
global_id = 1

for idx2, row2 in vista_p90_2.iterrows():
    for idx3, row3 in vista_p90_3.iterrows():
        if is_match(row2, row3):
            print(f"Matching {row2['id']} from vista-p90-2 with {row3['id']} from vista-p90-3")
            key2 = (row2.id, row2.sensor_id)
            key3 = (row3.id, row3.sensor_id)

            if key2 not in matches and key3 not in matches:
                print(f"Assigning global ID {global_id} to {key2} and {key3}")
                matches[key2] = global_id
                matches[key3] = global_id
                global_id += 1

print("Assigning global vehicle IDs to the original data")
group_data['global_vehicle_id'] = group_data.apply(
    lambda x: matches.get((x['id'], x['sensor_id']), None), axis=1)

group_data.to_csv('matched_vehicle_data.csv', index=False)
print(group_data.head(20))
