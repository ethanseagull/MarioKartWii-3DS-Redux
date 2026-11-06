# Changelog

## This fork, after 0.9.4

Changes in ethanseagull/MarioKartWii-3DS-Redux on top of upstream 0.9.4 (merged Nov 4, 2026; earlier fork changes were made on 0.8.14).

# Title and SD folder

- The game installs as its own title (`000400000F4B6000`), next to upstream's instead of over it, and keeps its data and saves in `sd:/3ds/MKWiiDS/` instead of `sd:/3ds/mkw-3ds/`. Move your saves across by hand (see the README).
- The HOME Menu shows it as "Mario Kart WiiDS" by ethanseagull, and the build's files are `MarioKartWiiDS-v<version>.cia` and `MarioKartWiiDS.3dsx` (the DIAG build's `MarioKartWiiDS-v<version>-DIAG.cia`).

# Performance (New 3DS)

- Pipelined race frames: the CPU builds the next frame while the GPU draws the last one.
- Merged the bloom composite into one pass and moved texture unit 2 to coordinate 1.
- Skipped the bottom-screen background fill where the map covers it.
- Limited shadow-volume shading to each shadow's screen rectangle.
- Moved the player's racer textures to VRAM ahead of the course's.
- Added the Wii's per-object draw distance for course objects.
- Drew course triangles in spatial order so culling blocks are compact.
- Drew the sky after the track's opaque surfaces instead of first.
- Loaded the position, item frame and lap HUD textures at half size.
- Kept Lakitu's joints, the pack indexes, course part lists and the last license cached instead of reloading them every race.
- Opponents load compact animation banks (`.dsku`), about 40% smaller.
- Sped up menu text animation by caching its results.
- Model draws with plain texture coordinates and none of the extra effects take a shorter vertex shader path.
- Course textures take VRAM by priority: the sky first, then the track textures that cover the most surface per byte.
- Menus draw like races: the render thread builds each frame's GPU commands while the next frame is prepared, and the GPU draws one frame while the next is built. Streamed menu movies upload through the frame's deferred writes. The pause pages still draw immediately.

# Racing and visuals

- GP, VS and Battle pause as on the Wii: the race stays frozen behind the menu without its HUD, with Continue Game and Quit Game. A controls diagram of the console shows above them (`tools/cook_pause_controls.py`, with Old and New 3DS photos supplied by smol_bug). Time Trials keeps its five buttons.
- CPU racers drift manually, as on the Wii, and after a fall their path restarts from their respawn point.
- Course, sky and course object textures are cooked to a texel budget instead of a flat 256 cap, so long road strips, skies and baked shadow maps keep more of the Wii's detail. The Old 3DS loads the larger ones one mip level down.
- New 3DS: every opponent keeps its full-detail kart, driver and wheels at any distance, and crowds and animated items run every tick, as in the Wii's single-player races.
- Blue Shells always leave forward, as on the Wii. A held-back stick, or a CPU throwing back at a racer behind it, launched them backwards off the item route.
- Each racer's item point (the route position Red and Blue Shells steer by) is kept as on the Wii: it starts at point 0, moves each frame to the closest point within two links, walks forward along the route when the kart is 12000 or more away, and is reset after a respawn. The nearest point anywhere on the course could lie on another stretch of road at hairpins and crossings.
- Blue Shells no longer turn back along the track at merges. When the leader's predicted route entered the next merge point from another branch, the shell took that branch backwards; it now keeps heading to the merge point.
- Cutouts that keep only fully opaque texels (Moonview Highway's guardrails and bridge lattice) test half coverage instead. The Wii's one-bit mip levels pass that test at every distance; ours carry partial alpha, so the rails broke up and vanished with distance.
- Wario's Gold Mine minecarts are clipped at each cart's own position. They were all hidden whenever the cart manager's placement was beyond draw distance, and the distant model used the near model's animation index.
- Mirror: the lower screen's map lines up with the racers again. It was flipped about the screen's centre while the racers were flipped about the map's centre, which the standings column moves off-centre.
- The first pause of a race no longer flashes black: while the pause page finishes loading, the race's last image stays on screen.
- No menu banners over a paused race: the Time Trials pause and the "Quit this race?" question drew the menu's header, footer and Back button.
- Time Trials start as on the Wii: no course preview, straight to the camera swinging round the kart and the countdown.
- Staff and saved ghosts replay their recorded run. Course objects and the karts kept moving through the course preview and the race's first two frames, so a ghost met Wario's Gold Mine's carts and other moving objects out of step with its run. Ghosts saved before this may not replay on courses with moving objects.
- Grumble Volcano's staff ghost finishes its 2:28.237 run. The lava fire snakes bounced along the wrong axis once they landed (Kinoko took the rest heading from the matrix's third column; the Wii uses the first), so on lap 2 a snake hit the ghost and it fell into the lava. All 64 staff ghosts and Kinoko's 56 reference ghosts now replay with every lap split exact in Kinoko on the PC.
- Grumble Volcano's falling ground follows the engine class, as on the Wii: each piece has its own times for 150cc, 100cc and 50cc (50cc falls latest). Every class used the 150cc times.
- GCN Peach Beach's Cataquacks walk their set paths in every Time Trial, as on the Wii, instead of chasing the racer, which could throw the ghost off its run.
- Menus: the Ghost page's time labels follow their own row (they all took the Staff Ghost row's press animation), the VS and Battle Rules OK button keeps its shape so "OK" is readable, and Settings values keep their letter spacing inside the buttons.
- Building no longer needs CFL_Res.dat: the game reads the console's own Mii resource. `--cfl` still packages a copy if you want.

# Settings

- The Wii's Settings button is back on the license screen, in the lower screen's top right corner as on the Wii (Up from the top row or Right from the right column). It opens the Wii's Settings page (its buttons, background and music) with this port's options, saved for the whole game in `settings.dat`:
  - Texture Quality: High keeps every texture at its cooked size; Low loads textures of 256x256 or more one mip level down.
  - Dynamic Shadows (moved here from License Settings; the first start takes the last license's choice).
  - Bloom: On or Off. Off also leaves bloom's VRAM to the course.
  - Opponent Detail: Full, or Low (distant opponents use the Wii's split-screen models).
  - Changes apply to the next race.
  - The upper screen shows a live sample of the highlighted option at its current value, drawn with the race's own code: the road close up at High or Low, the racer, seated still, with the dynamic or the plain shadow, with or without bloom, and at full or reduced detail.
  - Every option value's letters load with the page; "Opponent Detail: Full" had lost its "F" and "p".
- The license screen, title and main menu show the Wii's faint stripes over the blurred title picture on the lower screen, instead of the dark stripes the race menus use.

# Loading

- Pack entries are read as whole 512-byte SD sectors, and each read also takes the small entries right after it.
- Races after the first in a VS set or Grand Prix reuse the racers, items, effects, Lakitu and pause menu textures read by the previous load (New 3DS, RAM cache).
- Pausing keeps the racers loaded when the pause menu fits, so resuming is immediate.
- The pause menu's textures load with the race and stay until it ends, so START reads nothing from the SD card.
- The course preview camera is planned while the race sounds load.
- `tools/build.py` repacks race.pak and menu.pak in the order the game reads them (`tools/pakpack.c`, `tools/pak-order/`), LZ4-compressed (lossless); race.pak drops from 1.78 GB to about 1 GB. The game reads both pack formats, so older SD folders keep working.
- The SD folder is three packs and the 3DSX instead of about 1,800 loose files: movies and sound go into a new stream.pak, everything else into race.pak and menu.pak. The game's file opens look in the packs first, so no lookup scans a crowded folder. It still opens loose files, so older SD folders keep working.

# Movies

- The opening and both staff roll movies play every frame at 59.94 fps, as on the Wii (they were cooked at half rate).
- Movie frames are stored LZ4-compressed (lossless): all movies together take 796 MB instead of 889 MB, even with the opening and staff rolls at twice the frames, and the opening reads slightly less from the SD card per second than it did at 30 fps.
- The opening and staff roll sound is the movies' original DSP ADPCM, decoded as it plays, instead of decoded PCM: the same samples, about a third of the size.

# Time Trials

- Your own ghosts: every Time Trial records your inputs in the Wii's ghost format. A run that sets the course's best time (or the first finish on a course without a ghost) is saved as `ghosts/license-N/NN.rkg` in the SD folder, and the ghost page lists it as Your Ghost to race. Erasing a license erases its ghosts. Mii racers aren't recorded yet.
- Staff ghosts: race the disc's Staff Ghost on every course, and its Expert Staff Ghost once you beat the Staff Ghost's time. The ghost is translucent, can't be bumped and uses its own Triple Mushroom. Beating staff ghosts counts towards the retail unlocks that need them, and ghost races count as Ghost Races in your records.

# Fixes

- Time Trials no longer place Moonview Highway's Bob-omb cars, as on the Wii.
- Award ceremonies no longer fail at the driving scene: its effect bank is now cooked.
- The select and controller sounds no longer play a semitone flat.
- R tricks off boost ramps, half-pipes and zippers, not only trick surfaces.
- HOME banner: Mario has both eyes and the kart's textures are on the right parts.
- Races on a `tools/build.py` SD card no longer stop with "required drift definitions": the drift effects load from `race.pak`.
- A Lightning strike plays the driver's damage motion only during the spin, not for the whole time the racer is small.
- A trailing green or red shell no longer spins; thrown shells still do.
- Held and trailing items follow the ground under the kart instead of its body, so a bike's lean, a wheelie or a trick no longer tips them into the road.
- Up and down on the Circle Pad tilt the vehicle in the air, as on the Wii.
- Circle Pad steering: a radial deadzone, full lock a little before the rim, and full lock on diagonals.
- The course preview fades in and out as on the Wii. A skips it with a fade from its 61st frame; START no longer skips it. Its last shot holds until the fade ends, and the music starts with the first frame on screen instead of under the loading screen.

# Building

- `tools/build.py` builds the CIA and SD folder from your disc in one command, downloading the pinned converters and tools it needs.
- After a `git pull`, `tools/build.py` cooks again each stage whose tool or input files changed, and the stages after it, instead of keeping their old output.
- The HOME banner plays the game's Starting Grid jingle.
- The SD folder no longer holds loose copies of models that are already packed, or the cook receipts (`.json`), which the game never reads.
- Windows path, menu movie and menu texture fixes in the packaging tools.
- A diagnostic build with per-race profiling and an unattended 32-course benchmark (`tools/diag`).

## Upstream changes through 0.9.4 / HW143

Changes retained on top of the existing port, grouped by feature; withdrawn experiments, packaging-only revisions and unfinished investigations are excluded.

# Interface and menus

- Updated HOME banner conversion tools while keeping source inputs external to the repository.
- Bounded menu background residency, reclaimed retired models before replacement loads and retried failed backgrounds to prevent disappearing mode models.
- Restored the Grand Prix trophy's original diffuse lighting, weighted metal reflections and wood-base opacity.
- Restored the Time Trials stopwatch's metal and glass material combinations and reflection transforms.
- Restored the VS flagpole's environment mapping and the flag fabric's original lighting and tint.
- Restored both Battle balloon material combinations, including their reflections, intermediate clamps and mixed opacity.
- Restored the original title prompt, fades, looping glow and button confirmation feedback.
- Fixed missing title prompts, borders and confirmation controls caused by incomplete preloading.
- Restored menu movement, scaling, opacity and selection animations through the original layout hierarchy.
- Fixed transparent controls failing to reappear when deselected.
- Fixed duplicated Main Menu titles and incorrect bindings between text and glow layers.
- Rearranged menu content across both screens and corrected title, description, button and model placement.
- Fitted menu text proportionally instead of squeezing long labels horizontally.
- Corrected the size and proportions of Main Menu titles such as Single Player and Mario Kart Channel.
- Applied Main Menu confirmation squash and stretch to the glow while preserving the text and shadow proportions.
- Corrected the horizontal width of the Mario Kart Channel title on the bottom screen.
- Restored native menu fonts, text shadows, selection colors and button confirmation feedback.
- Fixed missing strokes, clipped text and incomplete digits caused by font atlas trimming.
- Standardized text clarity across race descriptions, Grand Prix results and Time Trials information.
- Corrected license card scaling, centering and title-screen preview proportions.
- Added the missing transitions between the title screen, license selection and Main Menu across both screens.
- Used the most recently selected existing license card on the title screen while retaining its light sweep.
- Added title background randomization under the established selection conditions.
- Corrected character and vehicle preview positions, name bars and performance lists.
- Used the corresponding original model animations and voices for character and vehicle confirmations.
- Corrected vehicle confirmation rotation counts, the final orientation and continuity when returning from drift selection.
- Fixed missing decorative-model entrance and exit motion at character selection boundaries.
- Kept identical decorative models continuous between adjacent pages.
- Restored selected cup emblems growing, rising and displaying their names.
- Centered cup emblems and labels using the icons' visible bounds.
- Corrected transition distances and return speeds between cup selection, course selection and confirmation.
- Fixed cup emblems briefly reappearing and top-screen text remaining during race confirmation.
- Added fades to the small preview movie, course name and full-screen movie on Grand Prix confirmation.
- Corrected character and vehicle movement around the confirmation window for random and sequential course selection.
- Restored the appropriate buttons and backgrounds for race confirmation, quitting and error messages.
- Fixed underlying menu text showing through message windows.
- Fixed bright edges and vertical seams in the scrolling striped background.
- Fixed mismatched upper and lower rings in the loading tire animation.
- Restored mode-specific pause controls, entrance animations and confirmation feedback.
- Fixed missing text, borders or backgrounds after repeatedly opening the pause menu.
- Prepared commonly used pause and quit-confirmation assets in advance.

# Licenses, rules and records

- Restored the omitted Mii Outfit B unlock-message entry so subsequent rewards display their correct text and models without changing saved unlock flags.
- Added persistent loading and saving for four license slots.
- Saved the most recently used license slot and restored it on startup.
- Saved each license identity with its corresponding Mii data.
- Retained backup loading and error messages for corrupted saves and failed writes.
- Combined shared license data reads to avoid reading the same save separately for all four slots at startup.
- Replaced save CRC32 calculations with a lookup table.
- Fixed early pages such as the title screen overwriting saved rules with defaults.
- Saved VS and Battle rules separately for each license and restored them on startup.
- Preserved valid rule selections when returning from the rules page, following the original behavior.
- Fixed failed rule saves updating the in-memory cache too early and preventing retries.
- Consolidated rule persistence when leaving the page.
- Fixed incomplete race-state resets when restarting certain modes.
- Corrected visibility and navigation for locked characters, vehicles and cups.
- Set the default engine class to 50cc when entering Grand Prix.
- Disabled the speedometer by default in every mode.
- Corrected transparency, animation and the stopwatch graphic behind Best Time on the Time Trials records page.
- Corrected Time Trials HUD placement, result descriptions and record digits.
- Saved dynamic shadow settings, LED mode and LED brightness per license.

# Miis and character models

- Preserved original color values in transparent face texels across race, menu and award expressions to prevent black facial patches.
- Replaced the question-mark result icon with the selected Mii portrait on individual and team result boards.
- Added native 3DS Mii selection and portrait rendering.
- Corrected license-card Mii portrait orientation, proportions, lighting and frame coverage.
- Fixed edited Miis incorrectly failing to save.
- Preserved native Mii hairstyle, facial-feature and accessory parameters.
- Opened the system Mii list after selecting Mii Outfit A or Mii Outfit B.
- Kept the selected racing Mii independent of the license's bound Mii.
- Added Mii racing bodies and seating poses for the corresponding size, gender and outfit.
- Added Mii vehicle resources for racing, vehicle selection, teams and award ceremonies.
- Used the appropriate native head, expressions, colors, portraits and voices for each Mii.
- Fixed Mii Outfit B unlock indices being confused with race character IDs.
- Fixed vehicle selection models disappearing after choosing a Mii.
- Restored Mii character and vehicle confirmation voices.
- Fixed the Mii race name incorrectly displaying Rosalina.
- Fixed vertical navigation targets not matching the visible Mii button positions.
- Waited for character selection to finish rendering after the Mii selector returned before playing the confirmation animation.
- Fixed Mii glasses and other transparent accessories rendering as opaque rectangles.
- Applied consistent transparency cutouts to Mii accessories in dynamic shadow and bloom captures.
- Restored Mii head lighting, highlights and Star color handling.
- Corrected the personal Mii's placement, neck connection and hair occlusion in the staff credits group photo.
- Fixed normal steering selecting incorrect facial expressions and blinking producing black faces.
- Fixed intermittent Toadette braid physics jitter and pose updates.
- Fixed black polygons around Funky Kong's trouser cuffs.
- Fixed Pokey's eyes appearing to have a white overlay.
- Fixed Rosalina's Luma appearing gray during the award ceremony.

# Race flow, cameras and CPU drivers

- Corrected the paired hallway Thwomps' return path and final turn angle on N64 Bowser's Castle.

- Stopped the lightning shrink timer from repeatedly triggering the damage animation after the initial reaction ends.
- Restored cup, course, mode and engine-class information during course intros.
- Extended intro text coverage to Battle arenas.
- Corrected clipping and placement of intro text, LAP, GO and result digits.
- Fixed the bottom-screen wide-angle intro camera clipping into terrain in certain scenes.
- Fixed camera clipping during the Dry Dry Ruins intro.
- Added smooth transitions for camera pitch changes during races.
- Smoothed entry into and exit from the Bullet Bill camera.
- Set the cannon-flight camera pitch to 1 degree.
- Distinguished normal airborne bike poses from cannon-flight poses.
- Fixed ordinary falls incorrectly using the horizontal cannon-flight pose.
- Added the rear-view camera and corrected its position, target and orientation.
- Increased the neutral dead zone for small analog stick movements.
- Adjusted trick and wheelie button mappings and added the corresponding ZR action.
- Required at least 80% downward stick input to throw an item backward.
- Fixed ordinary race collision feedback and certain obstacle hit callbacks being incorrectly cleared.
- Added driving, avoidance and stuck-recovery handling for the corresponding CPU difficulty levels.
- Corrected random CPU character and vehicle assignments using the original selection rules.
- Fixed some CPU drivers slowing down when the player braked.
- Corrected CPU recovery conditions near walls and flowing water.
- Corrected CPU trick eligibility across ordinary takeoff, jump pads and damage states.
- Fixed post-race driving on GCN DK Mountain following stale route state and repeatedly hitting walls.
- Restored GP and VS starting-grid order and inheritance of the previous race's finishing positions.
- Preserved the current starting positions when restarting a race.
- Fixed lap-sign Lakitu failing to appear in certain races.
- Added terrain clipping prevention for the wrong-way Lakitu.
- Fixed the wrong-way Lakitu retaining an abnormal floating trajectory after an uphill section.
- Added terrain clipping prevention to lap-sign Lakitu movement on complex terrain.

# Award ceremonies and staff credits

- Restored live diffuse lighting and rotating metal reflections for all nine award trophies while retaining their gold, silver and bronze material frames.
- Moved the award trophy and its emitter left with the result-page exit transition before hiding them off-screen.
- Added Grand Prix award scenes, camera rails, actors and trophy presentation.
- Added driving, expressions, crowds and scene animations to award ceremonies.
- Added missing rainbow, smoke, confetti and firework effects to award scenes.
- Fixed award scenes failing to load because of resource-directory ordering.
- Prepared award camera rails in advance to avoid allocating memory during camera cuts after the heap was locked.
- Corrected award content, text and team results across both screens.
- Reduced repeated processing of non-player models and lighting in award scenes.
- Fixed award scenes using the old incorrect rainbow material.
- Added the movies and subtitles for the normal and full endings' staff credits.
- Corrected staff subtitle control order, movie framing and group photo placement.
- Reduced both staff movies to approximately 30 fps while preserving the original audio and subtitle clock.
- Fixed crashes when entering a race after viewing the staff credits and returning to the title screen.

# Offline Battle modes

- Added offline Balloon Battle and Coin Runners across all ten arenas.
- Added the missing Battle mode, arena selection, team assignment and rules pages.
- Added CPU, vehicle, item, arena-order and win-target settings to Battle rules.
- Added balloon damage, stealing, loss, respawning and scoring.
- Restored balloon attachment points and physics constraints relative to the vehicle body pose.
- Fixed balloons shaking, jumping unpredictably or floating upward continuously.
- Blocked vehicle input while the racer was hidden after losing all balloons.
- Restored the blackout and respawn transition after losing all balloons.
- Added coin spawning, airdrops, drops, collection, stealing and recycling states.
- Added arbitration and deferred submission for competing attempts to collect the same coin.
- Fixed mismatches between coin collision positions, visual offsets and CPU target positions.
- Added coin ground binding, water states and arena-specific height offsets.
- Corrected stationary coin rendering and coin attachment to moving platforms.
- Added coin spawn scaling, rotation, collection and stealing effects.
- Restored the three-dimensional coin stack and column layout in Coin Runners.
- Added Battle CPU navigation, target selection, item use and starting behavior.
- Added live Battle rankings, individual scores, team totals and cumulative results.
- Fixed the same round's score being added again when re-entering the results flow.
- Restored controls, looping animations and team-score changes for both Battle result pages.
- Fixed overlapping Battle results, incorrect mode labels and engine-class text.
- Restored the appropriate Battle ending and team victory or defeat music.
- Changed the white fill of bottom-screen ranking digits to each team's color while preserving the black outline.
- Restored the original red and blue team-score colors on the top screen.
- Fixed overlapping character portraits on the Teams page.
- Fixed the Battle minimap to the full-arena view.
- Fixed forward-thrown Fake Item Boxes immediately hitting their owner in Battle.

# Items and course objects

- Destroyed Red Shells that remain locked onto a racer after that racer finishes.
- Added missing animation banks and state transitions for course objects.
- Fixed invalid poses caused by interpolation reading beyond the final animation frame.
- Added missing vehicle and item hit handling for objects including bats, Piranha Plants, cannonballs and rolling iron balls.
- Restored hit and recovery handling for wooden crates, factory blocks and other breakable objects.
- Added hiding, recovery and fragment resources for DK rocks destroyed by items.
- Restored target-facing behavior and hit feedback for the Piranha Plants on GCN Mario Circuit.
- Added reactions and recovery when Moonview Highway traffic was hit by powered-up racers or items.
- Restored Chain Chomp mouth morph animations.
- Added leaf-pile scattering, recovery and item drops.
- Restored the original hit feedback when powered-up racers contacted course obstacles.
- Added attachment relationships for moving platforms, bridges, disks and carried items.
- Corrected collision and CPU traversal when the bridge changed on DS Delfino Square.
- Corrected coins and items riding large rotating disks.
- Fixed SNES Ghost Valley walls losing collision on later laps without changing visually.
- Made SNES Ghost Valley ghosts face the camera using their billboard rules.
- Fixed stationary Thwomp Desert quicksand making vehicles and CPU drivers bounce continuously.
- Fixed Thwomp Desert quicksand incorrectly triggering jump tricks.
- Made vehicles sink into the terrain when falling into the center of Thwomp Desert.
- Fixed Grumble Volcano Fire Snakes disappearing before contacting a racer.
- Corrected Thunder Cloud model selection, scale and height above the player and opponents.
- Restored the Thunder Cloud face, lightning parts and flash timing.
- Corrected the overly dark lighting on the Thunder Cloud's lightning model.
- Expanded item warnings to cover more dangerous items and powered-up racers.
- Corrected item warning icon movement, direction and red flashes.
- Added the corresponding warning sounds beyond the existing shell alerts.
- Corrected Bullet Bill route progression and ground following on GBA Bowser Castle.
- Corrected Bullet Bill successor waypoint selection using the original branch order.
- Fixed Bullet Bill weaving and direction handling on sloped terrain.
- Made Bullet Bill ignore the appropriate obstacle collisions while retaining racer-hit feedback.
- Fixed items remaining held during cannon flight when they should have been cleared.
- Corrected dropped-item lifetimes, capacity-based recycling and landing states.

# Scene materials and rendering

- Restored the original white environment highlights in the Thunder Cloud's eyes.

- Restored the original lava color layers on N64 Bowser's Castle, including lava pillars and splash models.

- Restored spectators' original light-texture combinations and kept their lighting maps fixed.
- Restored the original internal lighting alpha for Peach Beach Nokis and a Bowser Castle crowd material to correct their dark appearance.
- Kept spectator crowds at their near-model appearance to prevent distance-dependent lighting changes.
- Restored the original GX lighting channels and environment reflections for racing characters and vehicles.
- Preserved source normals during GPU skinning and applied the corresponding normal transforms.
- Restored original course lighting, fog and supported layered material combinations.
- Fixed missing blue-purple layers and scrolling textures on half-pipe trick panels.
- Corrected boost-panel transparency, color combinations and independent texture coordinates.
- Corrected the sky-cloud blending equation on Peach Beach.
- Corrected Mario Circuit sky texture seams and Daisy Circuit cloud sampling.
- Restored the fixed sky rainbow and corrected its material binding.
- Fixed reflected Coconut Mall tree trunks failing to show their intended light color.
- Restored layered materials and environment reflections on supported statues.
- Fixed statues such as those on Daisy Circuit losing shading after lighting optimizations.
- Fixed Wario's Gold Mine flags to dark baked lighting.
- Used the same flag model at near and far distances to prevent color changes at LOD transitions.
- Fixed certain Maple Treeway models appearing white because of incorrect environment-reflection combinations.
- Added missing water materials on courses including Daisy Circuit, DS Delfino Square and Dry Dry Ruins.
- Restored original bloom parameters and adjusted composition strength and tint for the 3DS display.
- Adapted the relevant scene and menu presentation to the 400x240 aspect ratio.
- Corrected shared resources, depth and render state between the two stereoscopic views.
- Added and organized mipmap chains for static scene textures.
- Corrected distant-texture mip transitions and sampling artifacts on hardware.
- Applied targeted sampling adjustments to selected Mushroom Gorge and N64 DK's Jungle Parkway road surfaces and DS Delfino Square signs.
- Applied start-line sampling rules to the actual geometry without affecting grass and walls sharing the same texture atlas.
- Restored separate opaque and translucent drawing for course objects.
- Corrected color and alpha translation for grass, transparent objects and layered surfaces.
- Corrected live-screen capture, orientation, texture formats and stereo-eye state.
- Limited course-screen captures to scene content so countdowns and HUD elements were excluded.
- Restored missing Funky Stadium screens and Block Plaza dynamic displays.
- Added moving roads, bridges and course objects to the dynamic minimap.
- Corrected inverted minimap item icons, explosion-ring orientation and portrait outlines.
- Regenerated Bowser's Castle rotating-corridor minimap frames without the roof.

# Particles and visual effects

- Restored the original large and small statue fire effects and proximity triggers on N64 Bowser's Castle.

- Added missing vehicle particles for starts, burnouts, drifting, landings and road contact.
- Added missing activation and stopping conditions for leaf-area and chimney particles.
- Restored layered start smoke emission and rear-wheel contact conditions.
- Added CPU boost flames, drift sparks and wheelie wind effects.
- Corrected inward-drifting bike spark direction and placement along the inner side.
- Added missing bike brake smoke, scraping sparks and surface-specific dust.
- Added the relevant paths for shallow-water wakes, submerged bubbles, wheel sprays and spinning water splashes.
- Corrected the relationship between original water-contact states and particle emission positions.
- Added missing Fake Item Box fragments, power-up smoke and electrical effects.
- Restored the original texture combinations for Bob-omb and Blue Shell explosion cores.
- Restored fixed environment lighting on explosion cores to prevent them appearing too white after dynamic lighting was removed.
- Made blue smoke trigger on every valid Blue Shell explosion.
- Added missing lava-pillar splash effects on Bowser's Castle and Grumble Volcano.
- Added lava effects to grouped eruption vents in the latter part of Bowser's Castle.
- Added missing cannon smoke on GCN DK Mountain and Maple Treeway.
- Added the corresponding cannon-flight particles and speed presentation.
- Fixed phase jumps in Rainbow Road debris and star particles.
- Made boost panels and boost ramps clear ink from the screen.
- Added fades to Blooper's screen-darkening effect.

# Dynamic shadows and tire contact shadows

- Added dynamic silhouette shadows for the player's character, vehicle, tires and accessories.
- Used the course sun or the corresponding fixed light source to determine shadow direction.
- Fixed distant scenery bounds distorting the calculated light direction on courses such as Coconut Mall.
- Reversed Moonview Highway shadow direction and applied the established length cap.
- Added tire-layout contact patches for karts, three-wheelers and bikes.
- Adjusted contact patch length, width and placement for tire width and vehicle type.
- Fixed bike contact patches being too short.
- Matched contact patch color and coverage to the dynamic shadow.
- Fixed bright edges around tire contact patches and their boundaries with dynamic shadows.
- Added contact patch fades for takeoff, landing and state changes.
- Set the contact patch ground-clearance cutoff to 25.
- Disabled tire contact patches during pit, water and lava rescue states.
- Disabled tire contact patches when vehicle roll exceeded 40 degrees.
- Kept only the rear tire contact patch during bike wheelies.
- Disabled contact patches on specified vertically moving platforms and suspension bridges.
- Retained contact patches on working horizontal platforms such as those on Grumble Volcano and Toad's Factory.
- Limited contact patch projection stretching to prevent abnormal elongation in bowls.
- Capped dynamic shadow stretching at 1.5 times its reference length.
- Fixed a second, mirrored shadow appearing on the opposite side of DK Summit bowls.
- Fixed missing and fragmented shadows on flat roads, slopes, road decals and road-to-grass seams.
- Fixed missing dynamic shadows in hazard areas, on sand and on moving terrain.
- Fixed missing shadows and floating contact patches on Grumble Volcano moving platforms.
- Projected shadows onto the underlying terrain when driving through snow instead of onto the snow surface.
- Faded shadows smoothly when Rainbow Road sections changed road opacity.
- Added shadows for lap-sign, wrong-way and rescue Lakitu.
- Combined overlapping Lakitu and vehicle dynamic shadows without darkening the overlap twice.
- Fixed oversized Lakitu static shadows when dynamic shadows were disabled.
- Fixed bright borders where Lakitu and vehicle shadows merged.
- Fixed crashes when pressing START to open the pause menu after the shadow changes.
- Added a per-license dynamic shadow option enabled by default.

# Sound and music

- Made menu music replacement atomic and retired detached music instances to prevent overlapping BGM after settings changes.
- Kept audio automation on a continuous audio-thread clock during saves and loading while preserving game-controlled sound expiry.
- Raised audio refill priority and split scene reads into smaller pieces with active-stream refills between them.
- Restored the original voice ranges, fallback behavior and parameters for character and vehicle confirmations.
- Added missing sounds for damage, wall impacts, vehicle collisions and selected course obstacles.
- Added missing special collision sounds such as Coconut Mall traffic impacts.
- Restored course animal, moving-object and animation-linked sounds.
- Corrected cow and cowbell event routing and playback.
- Restored water ambience, splash sounds and selected short finish cues.
- Fixed ADPCM loop crackling in the second-lap cue and its rising high-note tail.
- Fixed duplicated opening-movie audio and asynchronous PCM queue ordering.
- Reduced the opening movie to approximately 30 fps while retaining its original audio track.
- Fixed opening audio repeating when multiple licenses existed.
- Adjusted the balance between sound effects and music to reduce excessive volume and distortion.
- Added the ordinary Bob-omb explosion echo and extended the audible range of the corresponding explosion sounds.
- Increased the playback priority of Lightning audio.
- Fixed item-box pickup sounds interrupting crowd cheers.
- Kept result music playing during loading after a race.
- Prevented ordinary sounds from the next race mixing into result-page loading.
- Managed sound instances by player category and event requirements.
- Added distance priority and stealing hysteresis for dense positional sound playback.
- Adjusted sound competition in response to truncation during simultaneous effects on Chain Chomp Wheel.
- Moved character voices and required sound resources to on-demand caching.
- Retained audible events outside the camera view, including item warnings from behind.

# Notification LED and settings

- Added item and key-event effects for the 3DS notification LED.
- Added Off, Items, Items + Events and Items + Events + Drifts modes.
- Added LED brightness adjustment from 50% to 100% and disabled LED effects by default.
- Fixed the brightness menu showing only 0 and 100.
- Made all game-owned LED effects fade out when ending or being canceled.
- Assigned drift effects the lowest LED priority.
- Prioritized Rainbow Road's final lap, item warnings, electric hits, other item effects, ordinary hits and drift effects in that order.
- Used pure red for ordinary damage while retaining item-specific damage colors.
- Used pure green for Green Shell hits.
- Used pure blue for Blue Shell launches and pure blue-to-white transitions for Blue Shell hits.
- Used yellow for Lightning, Thunder Cloud and electric-obstacle hits.
- Matched orange-red item-warning flashes to the corresponding warning cadence.
- Fixed item warnings overriding the following damage flash.
- Added an extra three-frame delay to damage LED effects.
- Added audio-start acknowledgments for boosts and item events with matching sounds.
- Unified Mushroom, Golden Mushroom and dropped Mushroom LEDs to trigger three frames after actual activation.
- Cycled Mushroom and boost-panel colors through yellow, pink, red and purple.
- Matched each single-boost LED cycle to the corresponding boost duration.
- Distinguished single Golden Mushroom activations from repeated rapid use.
- Added the corresponding sustained color cycles for Star, Mega Mushroom and Bullet Bill.
- Added gray to the existing Mega Mushroom and Bullet Bill palettes.
- Flashed orange-red with each POW cue.
- Matched Thunder Cloud LEDs to model flashes and followed activation with the yellow electric-hit effect.
- Triggered a white flash when a Blooper affecting the player was activated.
- Used a smooth rainbow gradient for Rainbow Road's final lap.
- Cycled drift colors while charged sparks were visible and flashed cyan or orange on release.
- Used the original texture's magenta countdown and blue start colors for the starting lights.
- Replaced the blue start effect with a red damage flash on burnout.
- Added two-color character confirmation effects sampled from specified locations on Standard Kart textures.
- Delayed the second character color until the first had fallen to 10% brightness.
- Applied the specified colors and ordering for King Boo, Toad, the Mario family, Yoshi and Toadette.
- Replaced nearly red character theme colors with pure red.

# Loading, memory and performance

- Reorganized VRAM, ordinary heap and LINEAR memory use for course textures and models.
- Reduced repeated character skinning, normal processing and rigid-part calculations.
- Added visibility culling for models, mesh sections and rigid parts.
- Reused identical poses and render state for consecutive matching materials.
- Shared shadow captures and related dynamic resources between both eyes at the same simulation tick.
- Reduced duplicate live-screen captures and intermediate buffers.
- Loaded effect and sound resources according to the current scene and actual dependencies.
- Avoided keeping unrelated effect libraries resident while retaining required recursive child-effect resources.
- Optimized spatial queries and submission for tire contact patches.
- Decoupled input and physics updates from contact patch drawing decisions.
- Disabled unnecessary runtime logs and diagnostic output in release builds.
- Added checkpoint-based indoor and outdoor visibility regions for Bowser's Castle.
- Added model fingerprints to castle region data and fell back to full drawing on a mismatch.
- Switched castle visibility regions at the entrance after the rotating corridor and near the final loop.
- Added the original low-detail character and vehicle models for opponents.
- Selected opponent LODs using distance, field of view and vehicle scale with switching hysteresis.
- Progressively omitted opponent front wheels and driver models at the farther LOD tiers.
- Kept full-detail models for the player, intros and results.
- Merged LOD models into race.pak and shared duplicate texture references.
- Fixed greatly increased course-loading, pause and resume times after LOD integration.
- Sampled player lighting textures every five simulation ticks and interpolated between samples.
- Used fixed lighting for static scenery and opponent materials that did not need dynamic updates.
- Restored full lighting for Item Boxes and Fake Item Boxes within the specified near range and blended the distance transition.
- Switched thrown items to their corresponding fixed-lighting path.
- Reduced temporary work in stable lighting-texture sampling and sound-distance sorting.
- Added adaptive animation update budgets with hysteresis for opponents and crowds.
- Exempted the player's own pose and head-turning animations from adaptive frame reduction.
- Retained per-tick updates for the player's effects while allowing lower update rates for opponents and scenery.
- Cached firebar texture phases within a bounded loading-time budget instead of synchronously reading each phase during racing.
- Adjusted worker-thread core assignments and priorities on New 3DS.
- Added atomic state rechecks after rendering-buffer wait timeouts.
- Cached identical vertex layouts, buffer configurations and texture-combiner register state to reduce duplicate GPU commands.
- Reused unchanged texture and floating-point uniform state to avoid submitting identical data again.
- Fixed missing shadow spatial-index restoration after resuming from pause making the rest of the race consistently heavier.
- Kept the static course loaded during pause while releasing reloadable drivers, vehicles and active-object resources.
- Moved the oversized renderer object off the main-thread stack to fix a startup crash.
- Prepared license portraits earlier to reduce loading stalls during the title transition across screens.
