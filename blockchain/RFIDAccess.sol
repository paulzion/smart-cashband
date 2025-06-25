// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract RFIDAccess {
    struct AccessRecord {
        string rfidId;
        uint256 timestamp;
        bool success;
        string fingerprintId;
    }
    
    AccessRecord[] public accessRecords;
    address public owner;
    
    event AccessAttempt(
        string rfidId,
        uint256 timestamp,
        bool success,
        string fingerprintId
    );
    
    constructor() {
        owner = msg.sender;
    }
    
    modifier onlyOwner() {
        require(msg.sender == owner, "Only owner can call this function");
        _;
    }
    
    function logAccess(
        string memory _rfidId,
        bool _success,
        string memory _fingerprintId
    ) public onlyOwner {
        accessRecords.push(AccessRecord({
            rfidId: _rfidId,
            timestamp: block.timestamp,
            success: _success,
            fingerprintId: _fingerprintId
        }));
        
        emit AccessAttempt(
            _rfidId,
            block.timestamp,
            _success,
            _fingerprintId
        );
    }
    
    function getAccessRecords() public view returns (AccessRecord[] memory) {
        return accessRecords;
    }
    
    function getAccessCount() public view returns (uint256) {
        return accessRecords.length;
    }
}