import pandas as pd
import folium
from folium.plugins import TimestampedGeoJson
import random

df = pd.read_csv("output/vehicle_objects_10min_nodimensions.csv")
df = df[
    (df['classification'] == 'VEHICLE')
]
df = df.dropna(subset=['map_matched_lat', 'map_matched_lon', 'timestamp'])

df['timestamp'] = pd.to_datetime(df['timestamp'])
df['time_str'] = df['timestamp'].dt.strftime('%Y-%m-%dT%H:%M:%S')

def random_color():
    return "#{:06x}".format(random.randint(0, 0xFFFFFF))

group_ids = df['group_id'].dropna().unique()
group_color_map = {gid: random_color() for gid in group_ids}

features = []
for (sensor, group_id), group in df.groupby(['sensor_id', 'group_id']):
    color = group_color_map.get(group_id, "#999999")  # fallback color for NaN
    for _, row in group.iterrows():
        features.append({
            "type": "Feature",
            "geometry": {
                "type": "Point",
                "coordinates": [row['map_matched_lon'], row['map_matched_lat']],
            },
            "properties": {
                "time": row['time_str'],
                "popup": f"{sensor} - ID {row['id']}",
                "icon": "circle",
                "iconstyle": {
                    "fillColor": color,
                    "fillOpacity": 1,
                    "stroke": "false",
                    "radius": 5
                }
            }
        })

timestamped_geojson = {
    "type": "FeatureCollection",
    "features": features
}

m = folium.Map(location=[df['map_matched_lat'].mean(), df['map_matched_lon'].mean()], zoom_start=18)
TimestampedGeoJson(
    data=timestamped_geojson,
    period='PT1S',
    duration='PT1S',
    add_last_point=False,
    transition_time=200,
    auto_play=True,
    loop=False,
    max_speed=1,
    loop_button=True,
    date_options='YYYY/MM/DD HH:mm:ss',
    time_slider_drag_update=True
).add_to(m)

m.save("output/plot_in_animation_vista1.html")
