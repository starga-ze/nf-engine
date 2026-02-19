async function fetchStats() {
  try {
    const res = await fetch("/api/v1/stats");
    const data = await res.json();

    document.getElementById("rx").textContent = data.rx_packets;
    document.getElementById("tx").textContent = data.tx_packets;
    document.getElementById("active").textContent = data.active_sessions;
  } catch (e) {
    console.error("Failed to fetch stats", e);
  }
}

setInterval(fetchStats, 1000);
fetchStats();

