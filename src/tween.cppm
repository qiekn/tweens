// Primary module interface for ck.tween.
//
// `import ck.tween;` pulls in every public partition. Internals (e.g. the
// future :manager partition) are imported here without re-export so they stay
// hidden from consumers.

export module ck.tween;

export import :ease;
