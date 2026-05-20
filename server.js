const express = require('express');
const path = require('path');

const app = express();
const port = 3000;

app.use(express.static(path.join(__dirname)));

app.listen(port, () => {
  console.log(`Serverul a pornit cu succes!`);
  console.log(`Acceseaza aplicatia WebGL aici: http://localhost:${port}`);
});