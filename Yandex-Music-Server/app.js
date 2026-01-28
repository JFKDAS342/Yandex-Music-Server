async function loadMusic() {
    const token = document.getElementById("token").value;

    const res = await fetch(`/api/music?token=${encodeURIComponent(token)}`);
    const data = await res.json();

    const root = document.getElementById("output");
    root.innerHTML = "";

    data.playlists.forEach(pl => {
        const plDiv = document.createElement("div");
        plDiv.className = "playlist";

        plDiv.innerHTML = `
            <h2>${pl.title} (${pl.track_count})</h2>
            <ul></ul>
        `;

        const ul = plDiv.querySelector("ul");

        pl.tracks.forEach(t => {
            const li = document.createElement("li");
            li.textContent =
                `${t.title} — ${t.artists} (${formatTime(t.duration_sec)})`;
            ul.appendChild(li);
        });

        root.appendChild(plDiv);
    });
}

function formatTime(sec) {
    const m = Math.floor(sec / 60);
    const s = sec % 60;
    return `${m}:${s.toString().padStart(2, "0")}`;
}
