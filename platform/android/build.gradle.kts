plugins {
    alias(libs.plugins.nexusPublishPlugin)
    alias(libs.plugins.kotlinter) apply false
    alias(libs.plugins.kotlinAndroid) apply false
    id("com.jaredsburrows.license") version "0.9.8" apply false
    id("maplibre.dependencies")
    id("maplibre.publish-root")
}


nexusPublishing {
    repositories {
        sonatype {
            stagingProfileId.set(extra["sonatypeStagingProfileId"] as String?)
            username.set(extra["ossrhUsername"] as String?)
            password.set(extra["ossrhPassword"] as String?)
            nexusUrl.set(uri("https://s01.oss.sonatype.org/service/local/"))
            snapshotRepositoryUrl.set(uri("https://s01.oss.sonatype.org/content/repositories/snapshots/"))
        }
    }
}

// Pin Kotlin stdlib and kotlinx-coroutines to versions compatible with the
// project's Kotlin compiler version, to prevent newer Android Studio installations
// from pulling in incompatible versions (e.g. kotlin-stdlib 2.2.20 or
// kotlinx-coroutines 1.11.0-rc01 compiled with Kotlin 2.2.x).
subprojects {
    configurations.all {
        resolutionStrategy.eachDependency {
            if (requested.group == "org.jetbrains.kotlin" &&
                (requested.name == "kotlin-stdlib" ||
                 requested.name == "kotlin-stdlib-common" ||
                 requested.name == "kotlin-stdlib-jdk7" ||
                 requested.name == "kotlin-stdlib-jdk8")) {
                useVersion(libs.versions.kotlin.get())
                because("Force Kotlin stdlib to match the project's compiler version")
            }
            if (requested.group == "org.jetbrains.kotlinx" &&
                requested.name.startsWith("kotlinx-coroutines")) {
                useVersion("1.9.0")
                because("kotlinx-coroutines 1.11.0-rc01 is compiled with Kotlin 2.2.x, incompatible with Kotlin 2.0.x")
            }
        }
    }
}
