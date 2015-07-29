//===- BuildFile.h ----------------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef LLBUILD_BUILDSYSTEM_BUILDFILE_H
#define LLBUILD_BUILDSYSTEM_BUILDFILE_H

#include "llbuild/Basic/Compiler.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace llbuild {
namespace buildsystem {

/// The type used to pass parsed properties to the delegate.
typedef std::vector<std::pair<std::string, std::string>> property_list_type;

class Task;

/// Abstract tool definition used by the build file.
class Tool {
  // DO NOT COPY
  Tool(const Tool&) LLBUILD_DELETED_FUNCTION;
  void operator=(const Tool&) LLBUILD_DELETED_FUNCTION;
  Tool &operator=(Tool&& rhs) LLBUILD_DELETED_FUNCTION;
    
  std::string name;

public:
  explicit Tool(const std::string& name) : name(name) {}
  virtual ~Tool();

  const std::string& getName() const { return name; }

  /// Called by the build file loader to configure a specified tool property.
  virtual bool configureAttribute(const std::string& name,
                                  const std::string& value) = 0;

  /// Called by the build file loader to create a task which uses this tool.
  ///
  /// \param name - The name of the task.
  virtual std::unique_ptr<Task> createTask(const std::string& name) = 0;
};

/// Each Target declares a name that can be used to reference it, and a list of
/// the top-level nodes which must be built to bring that target up to date.
class Target {
  /// The name of the target.
  std::string name;

  /// The list of node names that should be computed to build this target.
  std::vector<std::string> nodeNames;

public:
  explicit Target(std::string name) : name(name) { }

  const std::string& getName() const { return name; }

  std::vector<std::string>& getNodeNames() { return nodeNames; }
  const std::vector<std::string>& getNodeNames() const { return nodeNames; }
};

/// Abstract definition for a Node used by the build file.
class Node {
  // DO NOT COPY
  Node(const Node&) LLBUILD_DELETED_FUNCTION;
  void operator=(const Node&) LLBUILD_DELETED_FUNCTION;
  Node &operator=(Node&& rhs) LLBUILD_DELETED_FUNCTION;
    
  std::string name;

public:
  explicit Node(const std::string& name) : name(name) {}
  virtual ~Node();

  const std::string& getName() const { return name; }

  /// Called by the build file loader to configure a specified node property.
  virtual bool configureAttribute(const std::string& name,
                                  const std::string& value) = 0;
};

/// Abstract task definition used by the build file.
class Task {
  // DO NOT COPY
  Task(const Task&) LLBUILD_DELETED_FUNCTION;
  void operator=(const Task&) LLBUILD_DELETED_FUNCTION;
  Task &operator=(Task&& rhs) LLBUILD_DELETED_FUNCTION;
    
  std::string name;

public:
  explicit Task(const std::string& name) : name(name) {}
  virtual ~Task();

  const std::string& getName() const { return name; }

  /// Called by the build file loader to pass the list of input nodes.
  virtual void configureInputs(const std::vector<Node*>& inputs) = 0;

  /// Called by the build file loader to pass the list of output nodes.
  virtual void configureOutputs(const std::vector<Node*>& outputs) = 0;
                               
  /// Called by the build file loader to configure a specified task property.
  virtual bool configureAttribute(const std::string& name,
                                  const std::string& value) = 0;
};

class BuildFileDelegate {
public:
  virtual ~BuildFileDelegate();

  /// Called by the build file loader to report an error.
  //
  // FIXME: Support better diagnostics by passing a token of some kind.
  virtual void error(const std::string& filename,
                     const std::string& message) = 0;
  
  /// Called by the build file loader after the 'client' file section has been
  /// loaded.
  ///
  /// \param name The expected client name.
  /// \param version The client version specified in the file.
  /// \param properties The list of additional properties passed to the client.
  ///
  /// \returns True on success.
  virtual bool configureClient(const std::string& name,
                               uint32_t version,
                               const property_list_type& properties) = 0;

  /// Called by the build file loader to get a tool definition.
  ///
  /// \param name The name of the tool to lookup.
  /// \returns The tool to use on success, or otherwise nil.
  virtual std::unique_ptr<Tool> lookupTool(const std::string& name) = 0;

  /// Called by the build file loader to inform the client that a target
  /// definition has been loaded.
  virtual void loadedTarget(const std::string& name, const Target& target) = 0;

  /// Called by the build file loader to inform the client that a task
  /// has been fully loaded.
  virtual void loadedTask(const std::string& name, const Task& target) = 0;

  /// Called by the build file loader to get a node.
  ///
  /// \param name The name of the node to lookup.
  ///
  /// \param isImplicit Whether the node is an implicit one (created as a side
  /// effect of being declared by a task).
  virtual std::unique_ptr<Node> lookupNode(const std::string& name,
                                           bool isImplicit=false) = 0;
};

/// The BuildFile class supports the "llbuild"-native build description file
/// format.
class BuildFile {
public:
  // FIXME: This is an inefficent map, the string is duplicated.
  typedef std::unordered_map<std::string, std::unique_ptr<Node>> node_set;
  
  // FIXME: This is an inefficent map, the string is duplicated.
  typedef std::unordered_map<std::string, std::unique_ptr<Target>> target_set;

  // FIXME: This is an inefficent map, the string is duplicated.
  typedef std::unordered_map<std::string, std::unique_ptr<Task>> task_set;
  
  // FIXME: This is an inefficent map, the string is duplicated.
  typedef std::unordered_map<std::string, std::unique_ptr<Tool>> tool_set;

private:
  void *impl;

public:
  /// Create a build file with the given delegate.
  ///
  /// \arg mainFilename The path of the main build file.
  explicit BuildFile(const std::string& mainFilename,
                     BuildFileDelegate& delegate);
  ~BuildFile();

  /// Return the delegate the engine was configured with.
  BuildFileDelegate* getDelegate();

  /// @name Parse Actions
  /// @{

  /// Load the build file from the provided filename.
  ///
  /// This method should only be called once on the BuildFile, and it should be
  /// called before any other operations.
  ///
  /// \returns True on success.
  bool load();

  /// @}
  /// @name Accessors
  /// @{

  /// Get the set of declared nodes for the file.
  const node_set& getNodes() const;

  /// Get the set of declared targets for the file.
  const target_set& getTargets() const;

  /// Get the set of declared tasks for the file.
  const task_set& getTasks() const;

  /// Get the set of all tools used by the file.
  const tool_set& getTools() const;

  /// @}
};

}
}

#endif