async function loadPlaylists() {
    const token = document.getElementById("token").value;

    const res = await fetch(
        `/api/music?token=${encodeURIComponent(token)}&action=playlists`
    );

    const data = await res.json();
    console.log(data);

    if (data.status !== "ok") {
        alert(data.message);
        return;
    }

    const root = document.getElementById("output");
    root.innerHTML = "";

    data.playlists.forEach(pl => {
        const card = document.createElement("div");
        card.className = "playlist-card";

        card.innerHTML = `
            <img src="${pl.cover ?? 'https://via.placeholder.com/200'}">
            <h3>${pl.title}</h3>
            <p>${pl.track_count} треков</p>
        `;

        root.appendChild(card);
    });
}
