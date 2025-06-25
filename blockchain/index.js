const express = require('express');
const { ethers } = require('ethers');
const ABI = require('./contractABI');
const app = express();
app.use(express.json());

// Connect to local Hardhat network
const provider = new ethers.JsonRpcProvider('http://127.0.0.1:8545');
const wallet = new ethers.Wallet('0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80', provider);
const contract = new ethers.Contract('0x5FbDB2315678afecb367f032d93F642f64180aa3', ABI, wallet);

app.post('/log-access', async (req, res) => {
    try {
        const { rfidId, success, fingerprintId } = req.body;
        const tx = await contract.logAccess(rfidId, success, fingerprintId);
        await tx.wait();
        res.json({ success: true, txHash: tx.hash });
    } catch (error) {
        res.status(500).json({ success: false, error: error.message });
    }
});

app.listen(3000, () => {
    console.log('Server running on port 3000');
});