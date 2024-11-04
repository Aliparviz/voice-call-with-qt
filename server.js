const WebSocket = require('ws');

const PORT = 3000;
const wss = new WebSocket.Server({ port: PORT });
const clients = {};
const connectedClients = new Set();

console.log(`WebSocket server for P2P Call is running on ws://localhost:${PORT}`);

wss.on('connection', (ws) => {
  console.log('A new client connected.');

  ws.on('message', (message) => {
    const messageString = message.toString();
    console.log('Received:', messageString);

    let data;
    try {
      data = JSON.parse(messageString);
    } catch (error) {
      console.error('Invalid JSON received:', error);
      return;
    }

    if (data.type === 'register' && data.from) {
      clients[data.from] = ws;
      connectedClients.add(data.from);
      console.log(`Client ${data.from} registered successfully.`);
      logConnectedClients();
      return;
    }

    if (data.to && clients[data.to]) {
      console.log(`Forwarding message from ${data.from} to ${data.to}`);
      clients[data.to].send(messageString);
    } else {
      console.log(`Client ${data.to} not found. Broadcasting message to all clients.`);

      wss.clients.forEach((client) => {
        if (client !== ws && client.readyState === WebSocket.OPEN) {
          client.send(messageString);
        }
      });
    }
  });

  ws.on('close', () => {
    for (const id in clients) {
      if (clients[id] === ws) {
        console.log(`Client ${id} disconnected.`);
        delete clients[id];
        connectedClients.delete(id);
        logConnectedClients();
        break;
      }
    }
  });

  ws.on('error', (error) => {
    console.error('WebSocket error occurred:', error);
  });
});

function logConnectedClients() {
  const clientsList = Array.from(connectedClients).join(', ') || 'None';
  console.log(`Currently connected clients: ${clientsList}`);
}
