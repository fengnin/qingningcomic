import { FortuneCookie } from "./components/FortuneCookie";
import { BackgroundEffects } from "./components/BackgroundEffects";

export default function App() {
  return (
    <div className="min-h-screen w-full overflow-x-hidden relative">
      <BackgroundEffects />
      <div className="relative z-10">
        <FortuneCookie />
      </div>
    </div>
  );
}