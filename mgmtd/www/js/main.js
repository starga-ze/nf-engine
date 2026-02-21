import { fetchSessionStats, fetchEngineStats } from "./api.js";
import { renderDashboard } from "./dashboard.js";
import { renderSessions } from "./sessions.js";

function setupTabs() {
  document.querySelectorAll(".tab").forEach(btn => {
    btn.addEventListener("click", () => {
      document.querySelectorAll(".tab").forEach(t => t.classList.remove("active"));
      document.querySelectorAll(".tab-content").forEach(c => c.classList.remove("active"));

      btn.classList.add("active");
      document.getElementById(btn.dataset.tab).classList.add("active");
    });
  });
}

async function update() {
  try {
    const [engineStats, sessionStats] = await Promise.all([
      fetchEngineStats(),
      fetchSessionStats()
    ]);

    renderDashboard(engineStats, sessionStats.sessions);
    renderSessions(sessionStats.sessions);
  } catch (e) {
    console.error("Fetch failed", e);
  }
}

setupTabs();
setInterval(update, 1000);
update();
