import Foundation
import UIKit

struct CollectionMilestone: Identifiable {
    let id: String
    let title: String
    let description: String
    let requiredSubtypes: [String]
    let reward: String
    let rewardXP: Int

    var requiredCount: Int { requiredSubtypes.count }
}

final class CollectionTracker: ObservableObject {
    @Published var discoveredSubtypes: Set<String> = []

    let milestones: [CollectionMilestone] = [
        CollectionMilestone(
            id: "all_sources", title: "Shell Collector",
            description: "Discover all 4 Sources",
            requiredSubtypes: ["polyblep-saw", "polyblep-square", "noise-white", "fm-basic"],
            reward: "+50 XP to all sources", rewardXP: 50
        ),
        CollectionMilestone(
            id: "all_processors", title: "Coral Keeper",
            description: "Discover all 4 Processors",
            requiredSubtypes: ["svf-lp", "shaper-hard", "feedback", "svf-bp"],
            reward: "+50 XP to all processors", rewardXP: 50
        ),
        CollectionMilestone(
            id: "all_modulators", title: "Current Master",
            description: "Discover all 4 Modulators",
            requiredSubtypes: ["adsr-fast", "lfo-sine", "vel-map", "lfo-random"],
            reward: "+50 XP to all modulators", rewardXP: 50
        ),
        CollectionMilestone(
            id: "all_effects", title: "Tide Pool Elder",
            description: "Discover all 4 Effects",
            requiredSubtypes: ["delay-stereo", "chorus-lush", "reverb-hall", "dist-warm"],
            reward: "+50 XP to all effects", rewardXP: 50
        ),
        CollectionMilestone(
            id: "full_roster", title: "Reef Warden",
            description: "Discover all 16 core specimens",
            requiredSubtypes: SpecimenCatalog.coreSpecimens.map { $0.subtypeID },
            reward: "Unlock Deep Specimens + 200 XP", rewardXP: 200
        ),
    ]

    private let claimedKey = "obrix_collection_claimed"
    private var claimedMilestones: Set<String> {
        get { Set(UserDefaults.standard.stringArray(forKey: claimedKey) ?? []) }
        set { UserDefaults.standard.set(Array(newValue), forKey: claimedKey) }
    }

    /// Update discovered subtypes from reef + stasis
    func refresh(reefStore: ReefStore) {
        var discovered = Set<String>()
        for spec in reefStore.specimens.compactMap({ $0 }) {
            discovered.insert(spec.subtype)
        }
        for spec in reefStore.loadAllSpecimens() {
            discovered.insert(spec.subtype)
        }
        discoveredSubtypes = discovered
    }

    /// Check and return any newly completed milestones (not yet claimed)
    func checkCompletions() -> [CollectionMilestone] {
        var newCompletions: [CollectionMilestone] = []
        for milestone in milestones {
            if !claimedMilestones.contains(milestone.id) &&
               Set(milestone.requiredSubtypes).isSubset(of: discoveredSubtypes) {
                newCompletions.append(milestone)
            }
        }
        return newCompletions
    }

    /// Claim a milestone reward
    func claim(_ milestone: CollectionMilestone) {
        var claimed = claimedMilestones
        claimed.insert(milestone.id)
        claimedMilestones = claimed
        UINotificationFeedbackGenerator().notificationOccurred(.success)
    }

    func isClaimed(_ milestoneId: String) -> Bool {
        claimedMilestones.contains(milestoneId)
    }

    func progress(for milestone: CollectionMilestone) -> Int {
        milestone.requiredSubtypes.filter { discoveredSubtypes.contains($0) }.count
    }
}
