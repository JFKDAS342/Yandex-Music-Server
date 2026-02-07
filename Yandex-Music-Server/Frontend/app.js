let offset = 0;
const LIMIT = 30;

async function loadMusic(reset = true) {
    const token = document.getElementById("token").value;

    if (reset) {
        offset = 0;
        document.getElementById("liked-tracks").innerHTML = "";
    }

    const res = await fetch(
        `/api/music?token=${encodeURIComponent(token)}&offset=${offset}&limit=${LIMIT}`
    );

    const data = await res.json();

    if (data.status !== "ok") {
        alert(data.message || "Ошибка");
        return;
    }

    if (reset) {
        renderPlaylists(data.playlists);
    }

    appendLikedTracks(data.liked_tracks.items);

    offset += data.liked_tracks.items.length;

    
    if (data.liked_tracks.items.length < LIMIT) {
        document.getElementById("loadMoreBtn").style.display = "none";
    }
}

function renderPlaylists(playlists) {
    const root = document.getElementById("playlists");
    root.innerHTML = "";

    playlists.forEach(pl => {
        const div = document.createElement("div");
        div.className = "playlist-card";
        div.innerHTML = `
            <img src="${pl.cover || 'no-cover.png'}">
            <h3>${pl.title}</h3>
            <p>${pl.track_count} треков</p>
        `;
        root.appendChild(div);
    });
}

function appendLikedTracks(tracks) {
    const root = document.getElementById("liked-tracks");

    tracks.forEach(t => {
        const li = document.createElement("li");
        li.className = "track";
        li.textContent = `${t.count}. ${t.title} — ${t.artists.join(", ")}`;
        root.appendChild(li);
    });
}
