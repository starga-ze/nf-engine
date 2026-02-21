export async function fetchSessionStats() {
  const res = await fetch("/api/v1/stats/session");
  if (!res.ok) throw new Error("session fetch failed");
  return await res.json();
}

export async function fetchEngineStats() {
  const res = await fetch("/api/v1/stats/engine");
  if (!res.ok) throw new Error("engine fetch failed");
  return await res.json();
}
