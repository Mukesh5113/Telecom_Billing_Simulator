'use strict';

const $ = (sel) => document.querySelector(sel);

const modeEl = $('#mode');
const accountEl = $('#account');
const accountField = $('#accountField');
const runBtn = $('#runBtn');
const printBtn = $('#printBtn');
const statusEl = $('#status');
const resultsEl = $('#results');

const SYMBOL = '$';

// Format a plain amount string ("12.34" / "-2.00") as "$12.34" / "-$2.00".
function money(str) {
  const s = String(str == null ? '0' : str);
  if (s.startsWith('-')) return '-' + SYMBOL + s.slice(1);
  return SYMBOL + s;
}

function esc(s) {
  return String(s == null ? '' : s)
    .replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

function setStatus(msg, kind) {
  statusEl.textContent = msg || '';
  statusEl.className = 'status' + (kind ? ' ' + kind : '');
}

function isNeg(str) {
  return String(str).trim().startsWith('-');
}

// Group an invoice's charges by subscriber (account-level -> key "").
function groupBySubscriber(charges) {
  const groups = [];
  const index = new Map();
  for (const c of charges) {
    const key = c.subscriber_no || '';
    if (!index.has(key)) {
      index.set(key, groups.length);
      groups.push({ subscriber: key, items: [] });
    }
    groups[index.get(key)].items.push(c);
  }
  return groups;
}

function renderInvoice(inv) {
  const groups = groupBySubscriber(inv.charges || []);
  const t = inv.totals || {};

  let subsHtml = '';
  for (const g of groups) {
    const title = g.subscriber ? 'Subscriber ' + esc(g.subscriber) : 'Account-level charges';
    let rows = '';
    for (const c of g.items) {
      rows +=
        '<tr>' +
        '<td><span class="pill ' + esc(c.type) + '">' + esc(c.type) + '</span></td>' +
        '<td>' + esc(c.description) + '</td>' +
        '<td class="amt ' + (isNeg(c.amount) ? 'neg' : '') + '">' + money(c.amount) + '</td>' +
        '</tr>';
    }
    subsHtml +=
      '<div class="sub-block">' +
      '<p class="sub-title">' + title + '</p>' +
      '<table class="lines"><thead><tr><th>Type</th><th>Description</th><th>Amount</th></tr></thead>' +
      '<tbody>' + rows + '</tbody></table></div>';
  }

  const totalsHtml =
    '<div class="totals">' +
    row('Usage', money(t.usage)) +
    row('Recurring', money(t.recurring)) +
    row('Fees', money(t.fees)) +
    row('Discounts', money(t.discounts), isNeg(t.discounts)) +
    row('Tax', money(t.tax)) +
    '<div class="row grand"><span>GRAND TOTAL</span><span>' + money(t.grand_total) + '</span></div>' +
    '</div>';

  return (
    '<article class="invoice">' +
    '<div class="invoice-head">' +
    '<div class="who"><h3>' + esc(inv.name || 'Account') + '</h3>' +
    '<div class="ban">Account (BAN): ' + esc(inv.ban) + '</div></div>' +
    '<div class="doc"><span class="tag">INVOICE</span>' +
    '<div class="grand">' + money(t.grand_total) + '</div></div>' +
    '</div>' +
    subsHtml +
    totalsHtml +
    '</article>'
  );

  function row(label, val, neg) {
    return '<div class="row"><span>' + label + '</span><span' +
      (neg ? ' class="neg"' : '') + '>' + val + '</span></div>';
  }
}

function renderPrepaid(p) {
  let rows = '';
  for (const e of p.events || []) {
    const amt = e.kind === 'TOPUP' ? '+' + money(e.amount) : '-' + money(e.amount);
    const shown = e.rejected ? '<span class="rejected">rejected</span>' : (e.kind === 'TOPUP'
      ? '<span class="amt">' + amt + '</span>'
      : '<span class="amt neg">' + amt + '</span>');
    rows +=
      '<tr>' +
      '<td>' + esc(e.time) + '</td>' +
      '<td><span class="pill">' + esc(e.kind) + '</span></td>' +
      '<td>' + esc(e.description) + '</td>' +
      '<td class="amt">' + shown + '</td>' +
      '<td class="amt">' + money(e.balance_after) + '</td>' +
      '</tr>';
  }

  return (
    '<article class="invoice">' +
    '<div class="invoice-head">' +
    '<div class="who"><h3>Prepaid Subscriber ' + esc(p.subscriber_no) + '</h3>' +
    '<div class="ban">Account (BAN): ' + esc(p.ban) + '</div></div>' +
    '<div class="doc"><span class="tag">PREPAID</span>' +
    '<div class="grand">' + money(p.end_balance) + '</div></div>' +
    '</div>' +
    '<div class="bal-summary">' +
    '<div class="kpi"><div class="k">Opening balance</div><div class="v">' + money(p.start_balance) + '</div></div>' +
    '<div class="kpi"><div class="k">Closing balance</div><div class="v">' + money(p.end_balance) + '</div></div>' +
    '<div class="kpi"><div class="k">Rejected events</div><div class="v">' + esc(p.rejected_count) + '</div></div>' +
    '</div>' +
    '<table class="lines"><thead><tr><th>Time</th><th>Kind</th><th>Description</th><th>Amount</th><th>Balance</th></tr></thead>' +
    '<tbody>' + rows + '</tbody></table>' +
    '</article>'
  );
}

function renderResults(mode, data) {
  let html = '';
  if (mode === 'postpaid') {
    const invoices = (data && data.invoices) || [];
    if (!invoices.length) {
      html = '<div class="empty"><p>No postpaid invoices produced.</p></div>';
    } else {
      html = invoices.map(renderInvoice).join('');
    }
  } else {
    const list = (data && data.prepaid) || [];
    if (!list.length) {
      html = '<div class="empty"><p>No prepaid subscribers found.</p></div>';
    } else {
      html = list.map(renderPrepaid).join('');
    }
  }
  resultsEl.innerHTML = html;
}

async function generate() {
  const mode = modeEl.value;
  const account = accountEl.value.trim();

  runBtn.disabled = true;
  printBtn.disabled = true;
  setStatus('Running simulator...', 'busy');

  try {
    const params = new URLSearchParams({ mode });
    if (account && mode === 'postpaid') params.set('account', account);

    const resp = await fetch('/api/bill?' + params.toString());
    const payload = await resp.json();

    if (!resp.ok) {
      setStatus(payload.error || ('Request failed (' + resp.status + ')'), 'error');
      resultsEl.innerHTML = '<div class="empty"><p>Could not generate the bill.</p></div>';
      return;
    }

    renderResults(mode, payload.result);
    printBtn.disabled = false;
    if (payload.sample) {
      setStatus('Showing bundled SAMPLE output (build the C++ binary for live runs).', 'warn');
    } else {
      setStatus('Done. Rendered from: ' + (payload.binary || 'tbs'), '');
    }
  } catch (err) {
    setStatus('Error: ' + err.message, 'error');
  } finally {
    runBtn.disabled = false;
  }
}

modeEl.addEventListener('change', () => {
  accountField.style.display = modeEl.value === 'postpaid' ? '' : 'none';
});
runBtn.addEventListener('click', generate);
printBtn.addEventListener('click', () => window.print());
