import sys
import json
from yandex_music import Client

if len(sys.argv) < 2:
    print(json.dumps({"error": "token missing"}))
    exit(1)

token = sys.argv[1]

client = Client(token).init()

user_id = client.me.account.uid
playlists = client.users_playlists_list(user_id)

result_playlists = []

for pl in playlists:
    playlist_data = {
        "id": pl.playlist_id,
        "title": pl.title,
        "track_count": pl.track_count,
        "tracks": []
    }

    # загружаем треки плейлиста
    playlist_full = client.users_playlists(
        user_id,
        pl.kind
    )

    for item in playlist_full.tracks:
        track = item.track
        if not track:
            continue

        artists = ", ".join(a.name for a in track.artists)

        playlist_data["tracks"].append({
            "id": track.id,
            "title": track.title,
            "artists": artists,
            "duration_sec": track.duration_ms // 1000
        })

    result_playlists.append(playlist_data)

print(json.dumps({
    "status": "ok",
    "playlists": result_playlists
}, ensure_ascii=False))
