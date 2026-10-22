const API_BASE = window.TELEKOM_API_URL || localStorage.getItem("telekomApiUrl") || "/api";

async function request(path, options = {}) {
  const response = await fetch(`${API_BASE}${path}`, {
    headers: { "Content-Type": "application/json", ...options.headers },
    ...options,
  });
  if (!response.ok) {
    const body = await response.json().catch(() => ({}));
    throw new Error(body.message || `Request failed (${response.status})`);
  }
  return response.status === 204 ? null : response.json();
}

export const api = {
  dashboard: () => request("/dashboard"),
  subscribers: () => request("/subscribers"),
  blocks: () => request("/blocks"),
  addSubscriber: (name) => request("/subscribers", { method: "POST", body: JSON.stringify({ name }) }),
  createPayment: (payment) => request("/payments", { method: "POST", body: JSON.stringify(payment) }),
  runAudit: () => request("/blocks/audit", { method: "POST" }),
};

export { API_BASE };
