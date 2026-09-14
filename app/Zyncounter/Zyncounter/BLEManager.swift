//
//  BLEManager.swift
//  Zyncounter
//
//  Created by Ike Lyons on 8/12/26.
//

import Foundation
import CoreBluetooth
import os

@Observable
final class BLEManager: NSObject, CBCentralManagerDelegate {
    static let shared = BLEManager()

    // Must match UUIDs in esp code
    static let serviceUUID = CBUUID(string: "96BDE720-973D-4F43-820B-0CD2FF8B666C")
    static let characteristicUUID = CBUUID(string: "D5C94E7E-47E3-484D-897A-EA417B91B77A")
    static let timeCharacteristicUUID = CBUUID(string: "7677590E-7808-4E26-84E1-DA269B480206")

    static let restoreIdentifier = "com.zyncounter.centralManager"

    static let connectOptions: [String: Any] = [
        CBConnectPeripheralOptionNotifyOnConnectionKey: true,
        CBConnectPeripheralOptionNotifyOnDisconnectionKey: true,
        CBConnectPeripheralOptionNotifyOnNotificationKey: true,
    ]

    private let logger = Logger(subsystem: Bundle.main.bundleIdentifier ?? "Zyncounter", category: "BLEManager")

    var statusText = "Not connected"
    var receivedTimestamps: [Date] = []

    private var centralManager: CBCentralManager!
    private var zyncounter: CBPeripheral?

    private override init() {
        super.init()
        centralManager = CBCentralManager(delegate: self, queue: nil, options: [
            CBCentralManagerOptionRestoreIdentifierKey: Self.restoreIdentifier,
        ])
    }

    private func startScanning() {
        statusText = "Scanning..."
        centralManager.scanForPeripherals(withServices: [Self.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, willRestoreState dict: [String: Any]) {
        guard let restoredPeripherals = dict[CBCentralManagerRestoredStatePeripheralsKey] as? [CBPeripheral],
              let restored = restoredPeripherals.first else { return }

        logger.notice("Restored \(restored.name ?? "device", privacy: .public), state \(restored.state.rawValue)")
        zyncounter = restored
        restored.delegate = self
        statusText = "Restored connection to \(restored.name ?? "device")"

        if restored.state == .connected {
            restored.discoverServices([Self.serviceUUID])
        }
    }

    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        guard central.state == .poweredOn else {
            statusText = "Bluetooth is off"
            return
        }

        if zyncounter == nil {
            startScanning()
        } else if zyncounter?.state != .connected {
            central.connect(zyncounter!, options: Self.connectOptions)
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        central.stopScan()
        zyncounter = peripheral
        peripheral.delegate = self
        statusText = "Connecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral, options: Self.connectOptions)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        logger.notice("Connected to \(peripheral.name ?? "device", privacy: .public)")
        statusText = "Connected to \(peripheral.name ?? "device")"
        peripheral.discoverServices([Self.serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        statusText = "Reconnecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral, options: Self.connectOptions)
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        logger.notice("Disconnected from \(peripheral.name ?? "device", privacy: .public)")
        statusText = "Reconnecting to \(peripheral.name ?? "device")..."
        central.connect(peripheral, options: Self.connectOptions)
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
            peripheral.readValue(for: characteristic)
        }

        if let timeCharacteristic = service.characteristics?.first(where: { $0.uuid == Self.timeCharacteristicUUID }) {
            var epochSeconds = Int64(Date().timeIntervalSince1970)
            let data = Data(bytes: &epochSeconds, count: MemoryLayout<Int64>.size)
            peripheral.writeValue(data, for: timeCharacteristic, type: .withResponse)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        logger.notice("didUpdateValueFor fired at \(Date().description, privacy: .public)")
        guard characteristic.uuid == Self.characteristicUUID,
              let data = characteristic.value,
              data.count == MemoryLayout<Int64>.size else { return }

        let epochSeconds = data.withUnsafeBytes { $0.load(as: Int64.self) }
        guard epochSeconds != 0 else { return }

        receivedTimestamps.append(Date(timeIntervalSince1970: TimeInterval(epochSeconds)))
        peripheral.readValue(for: characteristic)
    }
}
