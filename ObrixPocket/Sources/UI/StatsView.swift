import SwiftUI

// MARK: - StatsView

struct StatsView: View {
    let stats = ReefStatsTracker.shared.allStats

    var body: some View {
        NavigationView {
            ScrollView {
                VStack(spacing: 0) {
                    ForEach(stats, id: \.0) { label, value, icon in
                        HStack(spacing: 12) {
                            Image(systemName: icon)
                                .font(.system(size: 14))
                                .foregroundColor(Color(hex: "1E8B7E").opacity(0.5))
                                .frame(width: 24)

                            Text(label)
                                .font(.custom("Inter-Regular", size: 13))
                                .foregroundColor(.white.opacity(0.6))

                            Spacer()

                            Text(value)
                                .font(.custom("JetBrainsMono-Bold", size: 14))
                                .foregroundColor(.white)
                        }
                        .padding(.horizontal, 20)
                        .padding(.vertical, 10)

                        Divider().background(Color.white.opacity(0.04))
                    }
                }
            }
            .background(Color(hex: "0E0E10"))
            .navigationTitle("Reef Statistics")
            .navigationBarTitleDisplayMode(.inline)
        }
    }
}
