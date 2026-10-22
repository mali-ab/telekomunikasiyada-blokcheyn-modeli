import { api } from "./api.js";

let subscribers = [];
let blocks = [];
let apiAvailable = false;

const $ = (selector, root = document) => root.querySelector(selector);
const $$ = (selector, root = document) => [...root.querySelectorAll(selector)];
const initials = (name = "") => String(name).trim().split(/\s+/).filter(Boolean).map((part) => part[0]).slice(0, 2).join("").toUpperCase() || "—";
const number = new Intl.NumberFormat("en-US");
const money = new Intl.NumberFormat("en-US", { maximumFractionDigits: 2 });
const escapeHtml = (value) => String(value ?? "—").replace(/[&<>"']/g, (character) => ({
  "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#039;",
})[character]);

function setText(selector, value) {
  const element = $(selector);
  if (element) element.textContent = value;
}

function isToday(timestamp) {
  if (!timestamp) return false;
  const datePart = String(timestamp).slice(0, 10);
  const now = new Date();
  const today = `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, "0")}-${String(now.getDate()).padStart(2, "0")}`;
  return datePart === today;
}

function showPage(id) {
  $$(".page").forEach((page) => page.classList.toggle("active", page.id === id));
  $$(".nav-item").forEach((item) => item.classList.toggle("active", item.dataset.page === id));
  $("#currentPage").textContent = id[0].toUpperCase() + id.slice(1);
  $("#sidebar").classList.remove("open");
  window.scrollTo({ top: 0, behavior: "smooth" });
}

function transactionRow(block) {
  return `<tr><td><div class="person"><span class="mini-avatar">${escapeHtml(initials(block.subscriber))}</span><strong>${escapeHtml(block.subscriber)}</strong></div></td><td><span class="service-badge">${escapeHtml(block.service)}</span></td><td><strong>${money.format(Number(block.amount) || 0)} TMT</strong></td><td><span class="block-link">#${number.format(Number(block.index) || 0)}</span></td><td><span class="status-badge">✓ Confirmed</span></td></tr>`;
}

function emptyRow(columns, title, detail) {
  return `<tr class="empty-row"><td colspan="${columns}"><span>${escapeHtml(title)}</span><small>${escapeHtml(detail)}</small></td></tr>`;
}

function setApiAvailability(available) {
  $$('[data-requires-api], #runAudit').forEach((element) => { element.disabled = !available; });
  $("#connectionState").classList.toggle("offline", !available);
}

function render() {
  const unavailableDetail = apiAvailable ? "New activity will appear here." : "Connect the backend API to load records.";
  $("#recentRows").innerHTML = blocks.length ? blocks.slice(0, 5).map(transactionRow).join("") : emptyRow(5, "No transactions", unavailableDetail);
  $("#subscriberRows").innerHTML = subscribers.length ? subscribers.map((subscriber, index) => `<tr><td>${index + 1}</td><td><div class="person"><span class="mini-avatar">${escapeHtml(initials(subscriber.name))}</span><strong>${escapeHtml(subscriber.name)}</strong></div></td><td>${escapeHtml(subscriber.internet)}</td><td>${escapeHtml(subscriber.iptv)}</td><td>${escapeHtml(subscriber.phone)}</td><td><strong>${money.format(Number(subscriber.balance) || 0)} TMT</strong></td><td><span class="status-badge">${escapeHtml(subscriber.status)}</span></td></tr>`).join("") : emptyRow(7, "No subscribers", unavailableDetail);
  $("#blockRows").innerHTML = blocks.length ? blocks.map((block) => `<tr><td><span class="block-link">#${number.format(Number(block.index) || 0)}</span></td><td>${escapeHtml(block.time)}</td><td><strong>${escapeHtml(block.subscriber)}</strong> • ${escapeHtml(block.service)} • ${money.format(Number(block.amount) || 0)} TMT</td><td><code title="${escapeHtml(block.hash)}">${escapeHtml(block.hash)}</code></td><td><span class="status-badge audit-result">Not audited</span></td></tr>`).join("") : emptyRow(5, "No ledger blocks", unavailableDetail);
  $("#paymentSubscriber").replaceChildren(...subscribers.map((subscriber) => new Option(subscriber.name, subscriber.name)));

  if (!apiAvailable) {
    setText('[data-metric="subscribers"]', "—");
    setText("#subscriberDetail", "API unavailable");
    setText('[data-metric="blocks"]', "—");
    setText("#blockDetail", "API unavailable");
    setText("#blockCount", "— blocks • SHA-256 proof of work");
    setText("#ledgerHealthValue", "— blocks");
    setText('[data-metric="payments"]', "—");
    setText("#paymentDetail", "API unavailable");
    return;
  }

  const todayBlocks = blocks.filter((block) => isToday(block.time) && Number(block.amount) > 0);
  const todayTotal = todayBlocks.reduce((total, block) => total + Number(block.amount || 0), 0);
  setText('[data-metric="subscribers"]', number.format(subscribers.length));
  setText("#subscriberDetail", "Loaded from PostgreSQL");
  setText('[data-metric="blocks"]', number.format(blocks.length));
  setText("#blockDetail", blocks.length ? `Latest block #${number.format(Number(blocks[0].index) || 0)}` : "No transaction blocks yet");
  setText("#blockCount", `${number.format(blocks.length)} blocks • SHA-256 proof of work`);
  setText("#ledgerHealthValue", `${number.format(blocks.length)} blocks`);
  setText('[data-metric="payments"]', `${money.format(todayTotal)} TMT`);
  setText("#paymentDetail", `${number.format(todayBlocks.length)} confirmed today`);
}

function openModal(id) { $("#modalBackdrop").classList.add("active"); $(`#${id}`).showModal(); }
function closeModals() { $$("dialog[open]").forEach((modal) => modal.close()); $("#modalBackdrop").classList.remove("active"); }
let toastTimer;
function toast(message, error = false) { const el = $("#toast"); el.textContent = message; el.className = `toast show${error ? " error" : ""}`; clearTimeout(toastTimer); toastTimer = setTimeout(() => el.className = "toast", 3500); }

async function loadBackend() {
  setApiAvailability(false);
  $("#connectionState b").textContent = "Connecting";
  $("#connectionState small").textContent = "Checking backend API";
  try {
    const [dashboard, subscriberData, blockData] = await Promise.all([api.dashboard(), api.subscribers(), api.blocks()]);
    if (Array.isArray(subscriberData)) subscribers = subscriberData;
    if (Array.isArray(blockData)) blocks = blockData;
    apiAvailable = true;
    $("#connectionState b").textContent = "Database connected";
    $("#connectionState small").textContent = "PostgreSQL • Live";
    setText('[data-metric="database"]', dashboard.database || "Online");
    setText("#databaseBadge", "Live");
    setText("#databaseDetail", "PostgreSQL connection established");
    setText("#healthSummary", "All core services available");
    $$('[data-health]').forEach((element) => { element.textContent = "Operational"; element.classList.remove("offline"); });
    setApiAvailability(true);
    render();
  } catch {
    apiAvailable = false;
    subscribers = [];
    blocks = [];
    $("#connectionState b").textContent = "API unavailable";
    $("#connectionState small").textContent = "Backend connection failed";
    setText('[data-metric="database"]', "Offline");
    setText("#databaseBadge", "Offline");
    setText("#databaseDetail", "Start telekom_web to use live data");
    setText("#healthSummary", "Backend services unavailable");
    $$('[data-health]').forEach((element) => { element.textContent = "Unavailable"; element.classList.add("offline"); });
    setApiAvailability(false);
    render();
  }
}

$$('.nav-item').forEach((item) => item.addEventListener("click", () => showPage(item.dataset.page)));
$$('[data-go]').forEach((item) => item.addEventListener("click", () => showPage(item.dataset.go)));
$$('[data-open]').forEach((item) => item.addEventListener("click", () => openModal(item.dataset.open)));
$$('[data-close]').forEach((item) => item.addEventListener("click", closeModals));
$("#modalBackdrop").addEventListener("click", closeModals);
$("#mobileMenu").addEventListener("click", () => $("#sidebar").classList.toggle("open"));
$("#subscriberSearch").addEventListener("input", (event) => { const query = event.target.value.toLowerCase(); $$("#subscriberRows tr").forEach((row) => row.hidden = !row.textContent.toLowerCase().includes(query)); });
$("#refreshSubscribers").addEventListener("click", async () => { await loadBackend(); toast("Subscriber list refreshed."); });
$("#nodeInfo").addEventListener("click", () => toast("Start telekom_system and enter the local port and optional peer address."));

$("#subscriberForm").addEventListener("submit", async (event) => {
  event.preventDefault(); const name = new FormData(event.currentTarget).get("name").trim();
  try {
    if (!apiAvailable) throw new Error("Backend API is unavailable.");
    subscribers.unshift(await api.addSubscriber(name));
    render(); closeModals(); event.currentTarget.reset(); toast("Subscriber account created.");
  } catch (error) { toast(error.message, true); }
});

$("#paymentForm").addEventListener("submit", async (event) => {
  event.preventDefault(); const values = Object.fromEntries(new FormData(event.currentTarget)); values.amount = Number(values.amount);
  try {
    if (!apiAvailable) throw new Error("Backend API is unavailable.");
    await api.createPayment(values); await loadBackend();
    render(); closeModals(); toast("Payment validated and stored on the blockchain.");
  } catch (error) { toast(error.message, true); }
});

$("#runAudit").addEventListener("click", async () => {
  const button = $("#runAudit"); button.disabled = true; button.textContent = "Auditing chain…";
  try {
    if (!apiAvailable) throw new Error("Backend API is unavailable.");
    await api.runAudit();
  }
  catch (error) { button.disabled = false; button.textContent = "✓ Run integrity audit"; toast(error.message, true); return; }
  $(".audit-state").classList.add("success"); $("#auditLabel").textContent = "Audit passed"; $("#auditDetail").textContent = "All loaded blocks passed the latest integrity check.";
  $$("#blockRows .audit-result").forEach((badge) => badge.textContent = "✓ Audit passed");
  button.disabled = false; button.textContent = "✓ Run integrity audit"; toast("Integrity audit passed — all blocks are valid.");
});

render(); loadBackend();
