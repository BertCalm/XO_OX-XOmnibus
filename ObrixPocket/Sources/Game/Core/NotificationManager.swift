import Foundation
import UserNotifications

final class NotificationManager {
    static let shared = NotificationManager()

    private init() {}

    /// Request notification permission
    func requestPermission() {
        UNUserNotificationCenter.current().requestAuthorization(options: [.alert, .sound, .badge]) { granted, error in
            if let error { print("[Notifications] Permission error: \(error)") }
        }
    }

    /// Schedule daily reminder — "Your reef misses you" after 2 days of inactivity
    func scheduleDormancyReminder() {
        let content = UNMutableNotificationContent()
        content.title = "Your reef needs you"
        content.body = "Your specimens are getting restless. Play to keep them healthy!"
        content.sound = .default

        // Trigger after 48 hours
        let trigger = UNTimeIntervalNotificationTrigger(timeInterval: 48 * 3600, repeats: false)
        let request = UNNotificationRequest(identifier: "dormancy_warning", content: content, trigger: trigger)

        UNUserNotificationCenter.current().add(request)
    }

    /// Schedule daily energy reminder — "Daily energy available!"
    func scheduleDailyEnergyReminder() {
        let content = UNMutableNotificationContent()
        content.title = "Daily Energy Available"
        content.body = "Play for 5 minutes to feed your reef and earn XP!"
        content.sound = .default

        // Schedule for 6 PM daily
        var dateComponents = DateComponents()
        dateComponents.hour = 18
        dateComponents.minute = 0
        let trigger = UNCalendarNotificationTrigger(dateMatching: dateComponents, repeats: true)
        let request = UNNotificationRequest(identifier: "daily_energy", content: content, trigger: trigger)

        UNUserNotificationCenter.current().add(request)
    }

    /// Schedule music catch reminder — "Your Song of the Day awaits"
    func scheduleMusicCatchReminder() {
        let content = UNMutableNotificationContent()
        content.title = "Song of the Day"
        content.body = "Your daily music catch has refreshed. What song will you choose?"
        content.sound = .default

        // Schedule for 9 AM daily
        var dateComponents = DateComponents()
        dateComponents.hour = 9
        dateComponents.minute = 0
        let trigger = UNCalendarNotificationTrigger(dateMatching: dateComponents, repeats: true)
        let request = UNNotificationRequest(identifier: "music_catch", content: content, trigger: trigger)

        UNUserNotificationCenter.current().add(request)
    }

    /// Reset dormancy timer (called on every app open)
    func resetDormancyTimer() {
        // Cancel the pending dormancy notification and reschedule
        UNUserNotificationCenter.current().removePendingNotificationRequests(withIdentifiers: ["dormancy_warning"])
        scheduleDormancyReminder()
    }

    /// Cancel all notifications
    func cancelAll() {
        UNUserNotificationCenter.current().removeAllPendingNotificationRequests()
    }
}
