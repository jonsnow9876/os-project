/**
 * App.jsx — Root component, dashboard layout
 *
 * Layout (3-column on wide screens):
 *   Left sidebar  → Controls + concept explainer
 *   Center main   → Thread state visualizer + timeline
 *   Right panel   → Live log console
 */

import React, { useState, useCallback } from 'react';
import ControlPanel    from './components/ControlPanel.jsx';
import ThreadVisualizer from './components/ThreadVisualizer.jsx';
import LogConsole      from './components/LogConsole.jsx';
import ConceptCards    from './components/ConceptCards.jsx';
import Header          from './components/Header.jsx';
import Timeline        from './components/Timeline.jsx';

export default function App() {
  // ── Simulation state ──────────────────────────────────────
  const [logs,       setLogs]       = useState([]);       // all parsed log entries
  const [isRunning,  setIsRunning]  = useState(false);    // simulation in flight
  const [hasRun,     setHasRun]     = useState(false);    // at least one run done
  const [config,     setConfig]     = useState({ threads: 3, iterations: 4 });
  const [error,      setError]      = useState(null);

  // ── Run simulation ────────────────────────────────────────
  const handleRun = useCallback(async (cfg) => {
    setIsRunning(true);
    setError(null);
    setLogs([]);
    setHasRun(false);

    try {
      const res = await fetch('/api/run', {
        method:  'POST',
        headers: { 'Content-Type': 'application/json' },
        body:    JSON.stringify(cfg)
      });

      const data = await res.json();

      if (!res.ok || !data.success) {
        setError(data.error || data.details || 'Simulation failed');
        setLogs(data.logs || []);
      } else {
        setLogs(data.logs || []);
        setConfig(data.config || cfg);
        setHasRun(true);
      }
    } catch (err) {
      setError('Cannot reach backend. Is the server running on port 5000?');
    } finally {
      setIsRunning(false);
    }
  }, []);

  return (
    <div style={styles.root}>
      {/* ── Top header bar ── */}
      <Header />

      {/* ── Main 3-column grid ── */}
      <div style={styles.grid}>

        {/* ── LEFT: controls + concept cards ── */}
        <aside style={styles.left}>
          <ControlPanel
            onRun={handleRun}
            isRunning={isRunning}
            defaultConfig={config}
          />
          <ConceptCards />
        </aside>

        {/* ── CENTER: thread states + timeline ── */}
        <main style={styles.center}>
          <ThreadVisualizer
            logs={logs}
            numThreads={config.threads}
            isRunning={isRunning}
            hasRun={hasRun}
          />
          {hasRun && (
            <Timeline logs={logs} numThreads={config.threads} />
          )}
        </main>

        {/* ── RIGHT: live log console ── */}
        <aside style={styles.right}>
          <LogConsole
            logs={logs}
            isRunning={isRunning}
            error={error}
          />
        </aside>

      </div>
    </div>
  );
}

// ── Styles ────────────────────────────────────────────────────
const styles = {
  root: {
    minHeight: '100vh',
    display:   'flex',
    flexDirection: 'column',
    padding: '0 0 32px 0',
  },
  grid: {
    flex: 1,
    display: 'grid',
    gridTemplateColumns: '280px 1fr 360px',
    gap: '16px',
    padding: '0 16px 0 16px',
    alignItems: 'start',
    '@media (max-width: 1100px)': {
      gridTemplateColumns: '1fr',
    }
  },
  left: {
    display: 'flex',
    flexDirection: 'column',
    gap: '16px',
  },
  center: {
    display: 'flex',
    flexDirection: 'column',
    gap: '16px',
    minWidth: 0,
  },
  right: {
    display: 'flex',
    flexDirection: 'column',
  }
};
