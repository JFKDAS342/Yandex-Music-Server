import sys
import io
sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')

from yandex_music import Client
import json

if len(sys.argv) < 4:
    print(json.dumps({
        "status": "error",
        "message": "no token / offset / limit"
    }, ensure_ascii=False))
    sys.exit(1)

token = sys.argv[1]
offset = int(sys.argv[2])   
limit = int(sys.argv[3])    

client = Client(token).init()


def fetch_playlists():
    playlists = client.users_playlists_list()
    playlist = []

    for pl in playlists:
        cover = None
        if pl.cover and pl.cover.uri:
            cover = "https://" + pl.cover.uri.replace("%%", "100x100")

        playlist.append({
            "title": pl.title,
            "track_count": pl.track_count,
            "cover": cover
        })

    return playlist


def fetch_liked_tracks(offset: int, limit: int):
    likes = client.users_likes_tracks()

    items = []
    i = offset

    while i < len(likes) and len(items) < limit:
        like = likes[i]
        i += 1

        try:
            track = like.fetch_track()
        except Exception:
            continue

        if not track:
            continue

        items.append({
            "count": i,
            "title": track.title,
            "artists": [a.name for a in track.artists],
            "duration": track.duration_ms // 1000
        })

    return {
        "items": items,
        "next_offset": i,
        "has_more": i < len(likes)
    }

def main():
    try:
        data = {
            "status": "ok",
            "playlists": fetch_playlists(),
            "liked_tracks": fetch_liked_tracks(offset, limit)
        }

        print(json.dumps(data, ensure_ascii=False))

    except Exception as e:
        print(json.dumps({
            "status": "error",
            "message": str(e)
        }, ensure_ascii=False))


if __name__ == "__main__":
    main()
