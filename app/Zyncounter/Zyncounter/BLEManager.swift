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
    // Must match UUIDs in esp code
    static let serviceUUID = CBUUID(string: "96BDE720-973D-4F43-820B-0CD2FF8B666C")
    static let characteristicUUID = CBUUID(string: "D5C94E7E-47E3-484D-897A-EA417B91B77A")
    static let timeCharacteristicUUID = CBUUID(string: "7677590E-7808-4E26-84E1-DA269B480206")

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
        statusText = "Reconnecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral, options: [
            CBConnectPeripheralOptionNotifyOnConnectionKey: true,
            CBConnectPeripheralOptionNotifyOnDisconnectionKey: true,
            CBConnectPeripheralOptionNotifyOnNotificationKey: true,
        ])
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        characteristicValue = ""
        statusText = "Reconnecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral, options: [
            CBConnectPeripheralOptionNotifyOnConnectionKey: true,
            CBConnectPeripheralOptionNotifyOnDisconnectionKey: true,
            CBConnectPeripheralOptionNotifyOnNotificationKey: true,
        ])
    }
}

extension BLEManager: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        for service in peripheral.services ?? [] {
            peripheral.discoverCharacteristics([Self.characteristicUUID, Self.timeCharacteristicUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let characteristic = service.characteristics?.first(where: { $0.uuid == Self.characteristicUUID }) {
            peripheral.setNotifyValue(true, for: characteristic)
        }

        if let timeCharacteristic = service.characteristics?.first(where: { $0.uuid == Self.timeCharacteristicUUID }) {
            var epochSeconds = Int64(Date().timeIntervalSince1970)
            let data = Data(bytes: &epochSeconds, count: MemoryLayout<Int64>.size)
            peripheral.writeValue(data, for: timeCharacteristic, type: .withResponse)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        guard characteristic.uuid == Self.characteristicUUID, let data = characteristic.value else { return }
        characteristicValue = String(data: data, encoding: .utf8) ?? data.map { String(format: "%02x", $0) }.joined()
    }
}
