//
//  BLEManager.swift
//  Zyncounter
//
//  Created by Ike Lyons on 8/12/26.
//

import Foundation
import CoreBluetooth

@Observable
final class BLEManager: NSObject, CBCentralManagerDelegate {
    // Must match SERVICE_UUID / CHARACTERISTIC_UUID in esp/hall-sensor-test/hall-sensor-test.ino
    static let serviceUUID = CBUUID(string: "96BDE720-973D-4F43-820B-0CD2FF8B666C")
    static let characteristicUUID = CBUUID(string: "D5C94E7E-47E3-484D-897A-EA417B91B77A")

    var statusText = "Not connected"
    var characteristicValue = ""

    private var centralManager: CBCentralManager!
    private var peripheral: CBPeripheral?

    override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil)
    }

    private func startScanning() {
        statusText = "Scanning..."
        centralManager.scanForPeripherals(withServices: [Self.serviceUUID])
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            startScanning()
        } else {
            statusText = "Bluetooth is off"
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        central.stopScan()
        self.peripheral = peripheral
        peripheral.delegate = self
        statusText = "Connecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        statusText = "Connected to \(peripheral.name ?? "device")"
        peripheral.discoverServices([Self.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        self.peripheral = nil
        startScanning()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        characteristicValue = ""
        self.peripheral = nil
        startScanning()
    }
}

extension BLEManager: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        for service in peripheral.services ?? [] {
            peripheral.discoverCharacteristics([Self.characteristicUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristic = service.characteristics?.first(where: { $0.uuid == Self.characteristicUUID }) else { return }
        peripheral.setNotifyValue(true, for: characteristic)
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard characteristic.uuid == Self.characteristicUUID, let data = characteristic.value else { return }
        characteristicValue = String(data: data, encoding: .utf8) ?? data.map { String(format: "%02x", $0) }.joined()
    }
}
