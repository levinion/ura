---@meta

--- @class ura.animation.definition
--- @field duration? number Milliseconds
--- @field easing? string|fun(t: number): number|table
--- @field enabled? boolean
--- @field properties? string[]
--- @field from? { scale?: number, opacity?: number }
--- @field to? { scale?: number, opacity?: number }

--- @class ura.animation
ura.animation = {}

--- @param name string
--- @param spec ura.animation.definition
function ura.animation.define(name, spec) end

--- @param name string
function ura.animation.undefine(name) end

--- @param name string
--- @return ura.animation.definition|nil
function ura.animation.get(name) end

--- @param fn fun(t: number): number
--- @return number[]
function ura.animation.sample_easing(fn) end

--- @param name string
--- @param event table|nil
--- @param fallback_enabled boolean|nil
--- @return table
function ura.animation.resolve(name, event, fallback_enabled) end
