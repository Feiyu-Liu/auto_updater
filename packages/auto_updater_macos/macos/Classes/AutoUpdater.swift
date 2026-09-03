import Cocoa
import FlutterMacOS
import Sparkle

extension SUAppcast {
    public func toDictionary() -> NSDictionary {
        let dict: NSDictionary = [
            "items": self.items.map({ item in
                return item.toDictionary()
            }),
        ]
        return dict;
    }
}

extension SUAppcastItem {
    
    
    public func toDictionary() -> NSDictionary {
        let dict: NSDictionary = [
            "versionString": self.versionString,
            "displayVersionString": self.displayVersionString,
            "fileURL": self.fileURL?.absoluteString ?? "",
            "contentLength": self.contentLength,
            "infoURL": self.infoURL?.absoluteString ?? "",
            "title":self.title ?? "",
            "dateString": self.dateString ?? "",
            "releaseNotesURL":self.releaseNotesURL?.absoluteString ?? "",
            "itemDescription":self.itemDescription ?? "",
            "itemDescriptionFormat": self.itemDescriptionFormat ?? "",
            "fullReleaseNotesURL": self.fullReleaseNotesURL ?? "",
            "minimumSystemVersion": self.minimumSystemVersion ?? "",
            "minimumOperatingSystemVersionIsOK": self.minimumOperatingSystemVersionIsOK,
            "maximumSystemVersion": self.maximumSystemVersion ?? "",
            "maximumOperatingSystemVersionIsOK": self.maximumOperatingSystemVersionIsOK,
            "channel": self.channel ?? "",
        ]
        return dict;
    }
}

public class AutoUpdater: NSObject, SPUUpdaterDelegate {
    private var userDriver: SPUStandardUserDriver?
    private var updater: SPUUpdater?
    private var feedURL: URL?
    private var hasStarted = false
    public var onEvent:((String, NSDictionary) -> Void)?
    
    override init() {
        super.init()
        let hostBundle: Bundle = Bundle.main
        
        userDriver = SPUStandardUserDriver(hostBundle: hostBundle, delegate: nil)
        updater = SPUUpdater(
            hostBundle: hostBundle,
            applicationBundle: hostBundle,
            userDriver: userDriver!,
            delegate: self
        )
    }
    
    public func feedURLString(for updater: SPUUpdater) -> String? {
        return feedURL?.absoluteString
    }

    public func setFeedURL(_ feedURL: URL?) throws {
        guard let feedURL else {
            throw NSError(domain: "auto_updater", code: 1, userInfo: [
                NSLocalizedDescriptionKey: "The update feed URL is missing."
            ])
        }
        self.feedURL = feedURL
        guard !hasStarted else { return }
        guard let updater else {
            throw NSError(domain: "auto_updater", code: 2, userInfo: [
                NSLocalizedDescriptionKey: "The updater is unavailable."
            ])
        }
        try updater.start()
        hasStarted = true
    }
    
    public func checkForUpdates() {
        updater?.checkForUpdates()
    }
    
    public func checkForUpdatesInBackground() {
        updater?.checkForUpdatesInBackground()
    }
    
    public func setScheduledCheckInterval(_ interval: Int) {
        updater?.updateCheckInterval = TimeInterval(interval)
    }
    
    // SPUUpdaterDelegate
    
    public func updater(_ updater: SPUUpdater, didAbortWithError error: Error) {
        let data: NSDictionary = [
            "error": error.localizedDescription,
        ]
        _emitEvent("error", data);
    }
    
    public func updater(_ updater: SPUUpdater, didFinishLoading appcast: SUAppcast) {
        let data: NSDictionary = [
            "appcast": appcast.toDictionary()
        ]
        _emitEvent("checking-for-update", data)
    }
    
    public func updater(_ updater: SPUUpdater, didFindValidUpdate item: SUAppcastItem) {
        let data: NSDictionary = [
            "appcastItem": item.toDictionary()
        ]
        _emitEvent("update-available", data)
    }
    
    public func updaterDidNotFindUpdate(_ updater: SPUUpdater, error: Error) {
        let data: NSDictionary = [
            "error": error.localizedDescription,
        ]
        _emitEvent("update-not-available", data)
    }
    
    public func updater(_ updater: SPUUpdater, didDownloadUpdate item: SUAppcastItem) {
        let data: NSDictionary = [
            "appcastItem": item.toDictionary()
        ]
        _emitEvent("update-downloaded", data)
    }
    
    public func updater(_ updater: SPUUpdater, willInstallUpdateOnQuit item: SUAppcastItem, immediateInstallationBlock immediateInstallHandler: @escaping () -> Void) -> Bool {
        let data: NSDictionary = [
            "appcastItem": item.toDictionary()
        ]
        _emitEvent("before-quit-for-update", data)
        return true
    }
    
    public func _emitEvent(_ eventName: String, _ data: NSDictionary) {
        let emit = { [weak self] in
            guard let self, let onEvent = self.onEvent else { return }
            onEvent(eventName, data)
        }
        if Thread.isMainThread {
            emit()
        } else {
            DispatchQueue.main.async(execute: emit)
        }
    }

    deinit {
        updater = nil
        userDriver = nil
    }
}
