const ABI = [
    {
        "inputs": [],
        "stateMutability": "nonpayable",
        "type": "constructor"
    },
    {
        "anonymous": false,
        "inputs": [
            {
                "indexed": false,
                "internalType": "string",
                "name": "rfidId",
                "type": "string"
            },
            {
                "indexed": false,
                "internalType": "uint256",
                "name": "timestamp",
                "type": "uint256"
            },
            {
                "indexed": false,
                "internalType": "bool",
                "name": "success",
                "type": "bool"
            },
            {
                "indexed": false,
                "internalType": "string",
                "name": "fingerprintId",
                "type": "string"
            }
        ],
        "name": "AccessAttempt",
        "type": "event"
    },
    {
        "inputs": [],
        "name": "getAccessCount",
        "outputs": [
            {
                "internalType": "uint256",
                "name": "",
                "type": "uint256"
            }
        ],
        "stateMutability": "view",
        "type": "function"
    },
    {
        "inputs": [],
        "name": "getAccessRecords",
        "outputs": [
            {
                "components": [
                    {
                        "internalType": "string",
                        "name": "rfidId",
                        "type": "string"
                    },
                    {
                        "internalType": "uint256",
                        "name": "timestamp",
                        "type": "uint256"
                    },
                    {
                        "internalType": "bool",
                        "name": "success",
                        "type": "bool"
                    },
                    {
                        "internalType": "string",
                        "name": "fingerprintId",
                        "type": "string"
                    }
                ],
                "internalType": "struct RFIDAccess.AccessRecord[]",
                "name": "",
                "type": "tuple[]"
            }
        ],
        "stateMutability": "view",
        "type": "function"
    },
    {
        "inputs": [
            {
                "internalType": "string",
                "name": "_rfidId",
                "type": "string"
            },
            {
                "internalType": "bool",
                "name": "_success",
                "type": "bool"
            },
            {
                "internalType": "string",
                "name": "_fingerprintId",
                "type": "string"
            }
        ],
        "name": "logAccess",
        "outputs": [],
        "stateMutability": "nonpayable",
        "type": "function"
    },
    {
        "inputs": [
            {
                "internalType": "uint256",
                "name": "",
                "type": "uint256"
            }
        ],
        "name": "accessRecords",
        "outputs": [
            {
                "internalType": "string",
                "name": "rfidId",
                "type": "string"
            },
            {
                "internalType": "uint256",
                "name": "timestamp",
                "type": "uint256"
            },
            {
                "internalType": "bool",
                "name": "success",
                "type": "bool"
            },
            {
                "internalType": "string",
                "name": "fingerprintId",
                "type": "string"
            }
        ],
        "stateMutability": "view",
        "type": "function"
    },
    {
        "inputs": [],
        "name": "owner",
        "outputs": [
            {
                "internalType": "address",
                "name": "",
                "type": "address"
            }
        ],
        "stateMutability": "view",
        "type": "function"
    }
];

module.exports = ABI;