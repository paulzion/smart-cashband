const { expect } = require("chai");
const { ethers } = require("hardhat");
const { anyValue } = require("@nomicfoundation/hardhat-chai-matchers/withArgs");

describe("RFIDAccess", function () {
  let rfidAccess;
  let owner;
  let otherAccount;

  beforeEach(async function () {
    // Get signers
    [owner, otherAccount] = await ethers.getSigners();

    // Deploy contract
    const RFIDAccess = await ethers.getContractFactory("RFIDAccess");
    rfidAccess = await RFIDAccess.deploy();
  });

  describe("Access Logging", function () {
    it("Should allow owner to log access", async function () {
      await expect(rfidAccess.logAccess("63:5A:59:31", true, "1"))
        .to.emit(rfidAccess, "AccessAttempt")
        .withArgs("63:5A:59:31", anyValue, true, "1");
    });

    it("Should not allow non-owner to log access", async function () {
      await expect(
        rfidAccess.connect(otherAccount).logAccess("63:5A:59:31", true, "1")
      ).to.be.revertedWith("Only owner can call this function");
    });

    it("Should correctly store access records", async function () {
      await rfidAccess.logAccess("63:5A:59:31", true, "1");
      
      const records = await rfidAccess.getAccessRecords();
      expect(records.length).to.equal(1);
      expect(records[0].rfidId).to.equal("63:5A:59:31");
      expect(records[0].success).to.equal(true);
      expect(records[0].fingerprintId).to.equal("1");
    });

    it("Should return correct access count", async function () {
      expect(await rfidAccess.getAccessCount()).to.equal(0);
      
      await rfidAccess.logAccess("63:5A:59:31", true, "1");
      expect(await rfidAccess.getAccessCount()).to.equal(1);
      
      await rfidAccess.logAccess("63:5A:59:31", false, "2");
      expect(await rfidAccess.getAccessCount()).to.equal(2);
    });
  });
});