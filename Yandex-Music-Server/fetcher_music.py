from yandex_music import Client
import sys
import json

if len(sys.argv) < 3:
    print(json.dumps({"status": "error", "message": "no token"}))
    sys.exit(1)

token = sys.argv[1]
action = sys.argv[2]

client = Client(token).init()

def fetch_playlists():
    playlists = client.users_playlists_list()
    result = []

    for pl in playlists:
        cover = None
        if pl.cover and pl.cover.uri:
            cover = "https://" + pl.cover.uri.replace("%%", "200x200")

        result.append({
            "title": pl.title,
            "track_count": pl.track_count,
            "cover": cover
        })

    return result

def main():
    try:
        if action == "playlists":
            data = fetch_playlists()
            print(json.dumps({
                "status": "ok",
                "playlists": data
            }, ensure_ascii=False))
        else:
            print(json.dumps({
                "status": "error",
                "message": "unknown action"
            }))
    except Exception as e:
        print(json.dumps({
            "status": "error",
            "message": str(e)
        }))

if __name__ == "__main__":
    main()
