#' Initialize the environment monitor
#' @param path Path to the JSON file where environment data should be written
#' @export
init_monitor <- function(path) {
  # Store the path in a package-internal environment or options
  options(qide.env_path = path)
  
  # Remove existing callback if any
  if ("qide_env_monitor" %in% getTaskCallbackNames()) {
    removeTaskCallback("qide_env_monitor")
  }
  
  # Register new callback
  addTaskCallback(function(...) {
    tryCatch({
      update_env()
    }, error = function(e) {
      message("Error in qide monitor: ", e$message)
    })
    return(TRUE)
  }, name = "qide_env_monitor")
  
  # Initial update
  update_env()
  invisible(NULL)
}

#' Update the environment file
#' @export
update_env <- function() {
  path <- getOption("qide.env_path")
  if (is.null(path)) {
    message("qide.env_path is not set")
    return(FALSE)
  }
  
  info <- get_env_info()
  
  # Write to file atomically (write to temp then move? or just write)
  # jsonlite::write_json is convenient
  if (requireNamespace("jsonlite", quietly = TRUE)) {
    tryCatch({
      jsonlite::write_json(info, path, auto_unbox = TRUE, pretty = FALSE)
      # message("Updated environment file: ", path) 
      return(TRUE)
    }, error = function(e) {
      message("Failed to write environment file: ", e$message)
      return(FALSE)
    })
  } else {
    message("jsonlite not available")
    return(FALSE)
  }
}

#' Get environment info as a list
#' @export
get_env_info <- function() {
  objs <- ls(envir = .GlobalEnv)
  
  if (length(objs) == 0) {
    return(list(
      objects = list(),
      types = list(),
      dim = list(),
      len = list()
    ))
  }
  
  vals <- mget(objs, envir = .GlobalEnv)
  
  # Helper to safely get class
  safe_class <- function(x) {
    tryCatch(class(x), error = function(e) "unknown")
  }
  
  # Helper to safely get dim
  safe_dim <- function(x) {
    d <- dim(x)
    if (is.null(d)) return(list()) # Return empty list for JSON {}
    return(d)
  }
  
  # Helper to safely get length
  safe_len <- function(x) {
    tryCatch(length(x), error = function(e) 0)
  }
  
  # Helper to safely get size
  safe_size <- function(x) {
    tryCatch(as.numeric(object.size(x)), error = function(e) 0)
  }
  
  sizes <- lapply(vals, safe_size)
  total_size <- sum(unlist(sizes))
  
  info <- list(
    objects = I(objs),
    types = lapply(vals, safe_class),
    dim = lapply(vals, safe_dim),
    len = lapply(vals, safe_len),
    size = sizes,
    total_size = total_size
  )
  
  return(info)
}

#' Clear the console
#' @export
clear <- function() {
  # In terminal, use ANSI escape sequence or system command
  if (interactive()) {
    cat("\033[2J\033[H")  # ANSI: clear screen and move cursor to home
  }
  invisible(NULL)
}
